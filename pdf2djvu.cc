#include <iostream>
#include <sstream>
#include <cerrno>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>

#include "goo/gmem.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "PDFDoc.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"
#include "SplashOutputDev.h"
#include "UTF8.h"

static int conf_dpi = 100;
static bool conf_antialias = false;
static char *conf_bg_slices = NULL;
static std::vector< std::pair<int, int> > conf_pages;
static char *file_name;

class Error
{
public:
  Error() : message("Unknown error") {};
  Error(const char* message) : message(message) {};
  Error(const std::string &message) : message(message) {};
  std::string &get_message()
  {
    return message;
  }
protected:
  std::string message;
};

class OSError : public Error
{
public:
  OSError() : Error("")
  {
    message += strerror(errno);
  }
};

class PagesParseError : public Error
{
public:
  PagesParseError() : Error("Unable to parse page numbers") {}
};

static std::string text_comment(int x, int y, int dx, int dy, int w, int h, Unicode *unistr, int len)
{
  std::ostringstream strstream;
  strstream
    << "# T " 
    <<  x << ":" <<  y << " " 
    << dx << ":" << dy << " "
    <<  w << "x" <<  h << "+" << x << "+" << y << " "
    << "(";
  static char buffer[8];
  while (len > 0 && *unistr == ' ')
    unistr++, len--;
  if (len == 0)
    return std::string();
  for (; len >= 0; len--, unistr++)
  {
    int seqlen = mapUTF8(*unistr, buffer, sizeof buffer);
    buffer[seqlen] = '\0';
    strstream << buffer;
  }
  strstream << ")" << std::endl;
  return strstream.str();
}

class MutedSplashOutputDev: public SplashOutputDev
{
private:
  std::vector<std::string> texts;
public:
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y, CharCode code, int n_bytes, Unicode *unistr, int len)
  {
    texts.push_back(text_comment(
      x / 72 * conf_dpi, 
      getBitmapHeight() - y / 72 * conf_dpi,
      dx / 72 * conf_dpi,
      dy / 72 * conf_dpi,
      dx / 72 * conf_dpi,
      10, // FIXME
      unistr,
      len
    ));
  }

  virtual GBool useDrawChar() { return gTrue; }

  MutedSplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA, GBool reverseVideoA, SplashColorPtr paperColorA, GBool bitmapTopDownA = gTrue, GBool allowAntialiasA = gTrue) :
    SplashOutputDev(colorModeA, bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA, allowAntialiasA)
  { }

  std::vector<std::string> &get_texts()
  {
    return texts;
  }

  void clear_texts()
  {
    texts.clear();
  }
};

static void usage()
{
  std::cerr 
    << "Usage: pdf2djvu [options] <pdf-file>" << std::endl
    << "Options:" << std::endl
    << " -d, --dpi=resolution"    << std::endl
    << " -q, --bg-slices=n,...,n" << std::endl
    << " -q, --bg-slices=n+...+n" << std::endl
    << "     --antialias"         << std::endl
    << " -p, --pages=..."         << std::endl
    << " -h, --help"              << std::endl
  ;
  exit(1);
}

void parse_pages(std::string s, std::vector< std::pair<int, int> > &result)
{
  int state = 0;
  int value[2] = { 0, 0 };
  for (std::string::iterator it = s.begin(); it != s.end(); it++)
  {
    if (('0' <= *it) && (*it <= '9'))
    {
      value[state] = value[state] * 10 + (int)(*it - '0');
      if (state == 0)
        value[1] = value[0];
    }
    else if (state == 0 && *it == '-')
    {
      state = 1;
      value[1] = 0;
    }
    else if (*it == ',')
    {
      if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
        throw PagesParseError();
      result.push_back(std::make_pair(value[0], value[1]));
      value[0] = value[1] = 0;
      state = 0;
    }
    else
      throw PagesParseError();
  }
  if (state == 0)
    value[1] = value[0];
  if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
    throw PagesParseError();
  result.push_back(std::make_pair(value[0], value[1]));
}

static bool read_config(int argc, char **argv)
{
  static struct option options [] =
  {
    { "dpi",        1, 0, 'd' },
    { "bg-slices",  1, 0, 'q' },
    { "antialias",  0, 0, 'A' },
    { "pages",      1, 0, 'p' },
    { "help",       0, 0, 'h' },
    { NULL,         0, 0, '\0' }
  };
  int optindex, c;
  while (true)
  {
    optindex = 0;
    c = getopt_long(argc, argv, "d:q:p:h", options, &optindex);
    if (c < 0)
      break;
    if (c == 0)
      c = options[optindex].val;
    switch (c)
    {
    case 'd': conf_dpi = atoi(optarg); break;
    case 'q': conf_bg_slices = optarg; break;
    case 'p':
      {
        try
        {
          parse_pages(optarg, conf_pages);
        }
        catch (PagesParseError ex)
        {
          return false;
        }
        break;
      }
    case 'A': conf_antialias = 1; break;
    case 'h': return false;
    default: ;
    }
  }
  if (optind == argc - 1)
    file_name = argv[optind];
  else
    return false;
  /* XXX
   * csepdjvu requires 25 <= dpi <= 144 000
   * djvumake requires 72 <= dpi <= 144 000
   * cpaldjvu requires 25 <= dpi <=   1 200 (but we don't use it)
   */
  if (conf_dpi < 72 || conf_dpi > 144000)
    return false;
  return true;
}

static void xsystem(std::string &command)
{
  int retval = system(command.c_str());
  if (retval == -1)
    throw OSError();
  else if (retval != 0)
  {
    std::ostringstream message;
    message << "system(\"";
    std::string::size_type i = command.find_first_of(' ', 0);
    message << command.substr(0, i);
    message << " ...\") failed with exit code " << (retval >> 8);
    throw Error(message.str());
  }
} 

static void xsystem(const std::ostringstream &command_stream)
{
  std::string command = command_stream.str();
  xsystem(command);
}

class TemporaryFile
{
public:
  inline const std::string &get_name() const
  {
    return name;
  }

  TemporaryFile()
  {
    char file_name_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    fd = mkstemp(file_name_buffer);
    if (fd == -1)
      throw OSError();
    name = std::string(file_name_buffer);
  }

  ~TemporaryFile()
  {
    if (close(fd) == -1)
      throw OSError();
    if (unlink(name.c_str()) == -1)
      throw OSError();
  }

  void fwrite(std::string &string)
  {
    fwrite(string.data(), string.size());
  }

  void fwrite(const void *buffer, size_t size)
  {
    if (write(fd, buffer, size) == -1)
      throw OSError();
  }

  void pass_to_stdout()
  {
    int fd = open(name.c_str(), O_RDONLY);
    if (fd == -1)
      throw OSError();
    while (1)
    {
      char buffer[1 << 12];
      int n = read(fd, buffer, sizeof buffer);
      if (n == 0)
        break;
      else if (n < 0)
        throw OSError();
      else
        if (write(STDOUT_FILENO, buffer, n) == -1)
          throw OSError();
    }
  } 

private:
  std::string name;
  int fd;
};

std::ostream &operator<<(std::ostream &out, const TemporaryFile &file)
{
  return out << file.get_name();
}

void operator+=(std::string &str, const TemporaryFile &file)
{
  str += file.get_name();
}


static int xmain(int argc, char **argv)
{
  if (!read_config(argc, argv))
    usage();
  GooString g_file_name(file_name);

#if POPPLER_VERSION < 6
  globalParams = new GlobalParams(NULL);
#else
  globalParams = new GlobalParams();
#endif
  if (!globalParams->setAntialias((char*)(conf_antialias ? "yes" : "no")))
    throw Error();

  PDFDoc *doc = new PDFDoc(&g_file_name);
  if (!doc->isOk())
    throw Error("Unable to load document");
  
  std::cerr << "About to process: " << doc->getFileName()->getCString() << std::endl;

  SplashColor paper_color;
  for (int i = 0; i < 3; i++)
    paper_color[i] = 0xff;

  SplashOutputDev *out1 = new SplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  MutedSplashOutputDev *outm = new MutedSplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  out1->startDoc(doc->getXRef());
  outm->startDoc(doc->getXRef());
  int n_pages = doc->getNumPages();
  TemporaryFile output_file;
  std::string djvm_command("/usr/bin/djvm -c ");
  TemporaryFile *page_files = new TemporaryFile[n_pages];
  djvm_command += output_file;
  if (conf_pages.size() == 0)
    conf_pages.push_back(std::make_pair(1, n_pages));
  for (std::vector< std::pair<int, int> >::iterator page_range = conf_pages.begin(); page_range != conf_pages.end(); page_range++)
  for (int n = page_range->first; n <= n_pages && n <= page_range->second; n++)
  {
    TemporaryFile &page_file = page_files[n - 1];
    std::cerr << "Page #" << n << ":" << std::endl;
    std::cerr << "- render with text" << std::endl;
    doc->displayPage(out1, n, conf_dpi, conf_dpi, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render without text" << std::endl;
    doc->displayPage(outm, n, conf_dpi, conf_dpi, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render done" << std::endl;
    SplashBitmap* bmp1 = out1->takeBitmap();
    SplashBitmap* bmpm = outm->takeBitmap();
    int width = bmp1->getWidth();
    int height = bmp1->getHeight();
    int row_size = bmp1->getRowSize();
    SplashColorPtr data1 = bmp1->getDataPtr();
    SplashColorPtr datam = bmpm->getDataPtr();
    SplashColorPtr row1, rowm;
    std::cerr << "- sep file" << std::endl;
    TemporaryFile sep_file;
    char buffer[1 << 10];
    int len = sprintf(buffer, "R6 %d %d 216\n", width, height);
    sep_file.fwrite(buffer, len);
    std::cerr << "- rle palette" << std::endl;
    for (int r = 0; r < 6; r++)
    for (int g = 0; g < 6; g++)
    for (int b = 0; b < 6; b++)
    {
      char buffer[] = { 51 * r, 51 * g, 51 * b };
      sep_file.fwrite(buffer, 3);
    }
    row1 = data1;
    rowm = datam;
    std::cerr << "- generate rle" << std::endl;
    bool has_background = false;
    bool has_foreground = false;
    bool has_text = false;
    for (int y = 0; y < height; y++)
    {
      SplashColorPtr p1 = row1;
      SplashColorPtr pm = rowm;
      int new_color, color = 0xfff;
      int length = 0;
      for (int x = 0; x < width; x++)
      {
        if (!has_background && (pm[0] & pm[0] & pm[0]) != 0xff)
          has_background = true;
        if (p1[0] != pm[0] || p1[1] != pm[1] || p1[2] != pm[2])
        {
          if (!has_foreground && (p1[0] || p1[0] || p1[0]))
            has_foreground = true;
          new_color = (p1[2] / 51) + 6 * ((p1[1] / 51) + 6 * (p1[0] / 51));
        }
        else
          new_color = 0xfff;
        if (color == new_color)
          length++;
        else
        {
          int item = (color << 20) + length;
          for (int i = 0; i < 4; i++)
          {
            char c = item >> ((3 - i) * 8);
            sep_file.fwrite(&c, 1);
          }
          color = new_color;
          length = 1;
        }
        p1 += 3;
        pm += 3;
      }
      row1 += row_size;
      rowm += row_size;
      int item = (color << 20) + length;
      for (int i = 0; i < 4; i++)
      {
        char c = item >> ((3 - i) * 8);
        sep_file.fwrite(&c, 1);
      }
    }
    delete bmpm;
    if (has_background)
    {
      std::cerr << "- generate ppm" << std::endl;
      len = sprintf(buffer, "P6 %d %d 255\n", width, height);
      sep_file.fwrite(buffer, len);
      row1 = data1;
      for (int y = 0; y < height; y++)
      {
        sep_file.fwrite(row1, width * 3);
        row1 += row_size;
      }
    }
    delete bmp1;
    {
      std::cerr << "- add text layer" << std::endl;
      std::vector<std::string> &texts = outm->get_texts();
      for (std::vector<std::string>::iterator it = texts.begin(); it != texts.end(); it++)
      {
        if (it->size() == 0)
          continue;
        sep_file.fwrite(*it);
        has_text = true;
      }
      outm->clear_texts();
    }
    std::cerr << "- about to call csepdjvu" << std::endl;
    std::ostringstream csepdjvu_command;
    csepdjvu_command << "/usr/bin/csepdjvu";
    csepdjvu_command << " -d " << conf_dpi;
    if (conf_bg_slices)
      csepdjvu_command << " -q " << conf_bg_slices;
    csepdjvu_command << " " << sep_file << " " << page_file;
    std::string csepdjvu_command_str = csepdjvu_command.str();
    xsystem(csepdjvu_command_str);
    djvm_command += " ";
    djvm_command += page_file;
    std::cerr << "- done!" << std::endl;
    std::cerr << "- about to recompress Sjbz" << std::endl;
    /* XXX csepdjvu produces ridiculously large Sjbz chunks. */
    TemporaryFile rle_file, sjbz_file, fgbz_file, bg44_file, sed_file;
    {
      std::ostringstream command;
      command << "/usr/bin/ddjvu -format=rle -mode=mask " << page_file << " " << rle_file;
      xsystem(command);
    }
    if (has_background || has_foreground)
    { 
      std::ostringstream command;
      command << "/usr/bin/djvuextract " << page_file << " FGbz=" << fgbz_file << " BG44=" << bg44_file;
      xsystem(command);
    }
    if (has_text)
    {
      std::ostringstream command;
      command << "/usr/bin/djvused " << page_file << " -e output-txt > " << sed_file;
      xsystem(command);
    }
    {
      std::ostringstream command;
      command << "/usr/bin/cjb2 " << rle_file << " " << sjbz_file;
      xsystem(command);
    }
    {
      std::ostringstream command;
      command 
        << "/usr/bin/djvumake"
        << " " << page_file
        << " INFO=" << width << "," << height << "," << conf_dpi
        << " Sjbz=" << sjbz_file;
      if (has_foreground || has_background)
        command
          << " FGbz=" << fgbz_file
          << " BG44=" << bg44_file;
      xsystem(command);
    }
    if (has_text)
    {
      std::ostringstream command;
      command << "/usr/bin/djvused " << page_file << " -s -f " << sed_file;
      xsystem(command);
    }
    std::cerr << "- done!" << std::endl;
  }
  std::cerr << "About to call djvm" << std::endl;
  xsystem(djvm_command);
  std::cerr << "Done!" << std::endl;

  output_file.pass_to_stdout();
  
  std::cerr << "About to remove temporary files" << std::endl;
  delete[] page_files;
  std::cerr << "Done!" << std::endl;

  return 0;
}

int main(int argc, char **argv)
{
  try
  {
    xmain(argc, argv);
  }
  catch (Error ex)
  {
    std::cerr << ex.get_message() << std::endl;
    exit(1);
  }
}

// vim:ts=2 sw=2 et
