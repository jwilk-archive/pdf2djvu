#include <iostream>
#include <sstream>
#include <cerrno>

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

class MutedSplashOutputDev: public SplashOutputDev
{
public:
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode *u, int uLen)
  {
    return;
  }
  virtual GBool useDrawChar() { return gTrue; }

  MutedSplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA, GBool reverseVideoA, SplashColorPtr paperColorA, GBool bitmapTopDownA = gTrue, GBool allowAntialiasA = gTrue) :
    SplashOutputDev(colorModeA, bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA, allowAntialiasA)
  { }
};

static void usage()
{
  std::cerr 
    << "Usage: pdf2djvu [options] <pdf-file>" << std::endl
    << "Options:" << std::endl
    << " -d, --dpi=resolution"    << std::endl
    << " -q, --bg-slices=n,...,n" << std::endl
    << " -q, --bg-slices=n+...+n" << std::endl
    << " -h, --help"              << std::endl
  ;
  exit(1);
}

static void pass_to_stdout(const std::string &file_name)
{
  int fd = open(file_name.c_str(), O_RDONLY);
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


static int conf_dpi = 100;
static char *conf_bg_slices = NULL;
static char *file_name;

static bool read_config(int argc, char **argv)
{
  static struct option options [] =
  {
    { "dpi",        1, 0, 'd' },
    { "bg-slices",  1, 0, 'q' },
    { "help",       0, 0, 'h' },
    { NULL,         0, 0, '\0' }
  };
  int optindex, c;
  while (true)
  {
    optindex = 0;
    c = getopt_long(argc, argv, "d:q:h", options, &optindex);
    if (c < 0)
      break;
    if (c == 0)
      c = options[optindex].val;
    switch (c)
    {
    case 'd': conf_dpi = atoi(optarg); break;
    case 'q': conf_bg_slices = optarg; break;
    case 'h': return false;
    default: ;
    }
  }
  if (optind == argc - 1)
    file_name = argv[optind];
  else
    return false;
  if (conf_dpi < 25)
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

class TemporaryFile
{
public:
  const std::string &get_name()
  {
    return name;
  }

  TemporaryFile()
  {
    char file_name_buffer[] = "/tmp/djvu2pdf.XXXXXX";
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

  void fwrite(const void *buffer, size_t size)
  {
    if (write(fd, buffer, size) == -1)
      throw OSError();
  }

private:
  std::string name;
  int fd;
};

static int xmain(int argc, char **argv)
{
  if (!read_config(argc, argv))
    usage();
  GooString g_file_name(file_name);

  globalParams = new GlobalParams(NULL);

  PDFDoc *doc = new PDFDoc(&g_file_name);
  if (!doc->isOk())
    throw Error("Unable to load document");
  
  std::cerr << "About to process: " << doc->getFileName()->getCString() << std::endl;

  SplashColor paper_color;
  for (int i = 0; i < 3; i++)
    paper_color[i] = 0xff;

  SplashOutputDev *out1 = new SplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  SplashOutputDev *outm = new MutedSplashOutputDev(splashModeRGB8, 4, gFalse, paper_color);
  out1->startDoc(doc->getXRef());
  outm->startDoc(doc->getXRef());
  int n_pages = doc->getNumPages();
  TemporaryFile output_file;
  std::string djvm_command("/usr/bin/djvm -c ");
  TemporaryFile *page_files = new TemporaryFile[n_pages];
  djvm_command += output_file.get_name();
  for (int n = 1; n <= n_pages; n++)
  {
    TemporaryFile &page_file = page_files[n - 1];
    std::cerr << "Page #" << n << ":" << std::endl;
    std::cerr << "- render with text" << std::endl;
    doc->displayPage(out1, n, conf_dpi, conf_dpi, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render without text" << std::endl;
    doc->displayPage(outm, n, conf_dpi, conf_dpi, 0, gTrue, gFalse, gFalse);
    std::cerr << "- render done" << std::endl;
    SplashBitmap* bmp1 = out1->getBitmap();
    SplashBitmap* bmpm = outm->getBitmap();
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
    for (int y = 0; y < height; y++)
    {
      SplashColorPtr p1 = row1;
      SplashColorPtr pm = rowm;
      int new_color, color = 0xfff;
      int length = 0;
      for (int x = 0; x < width; x++)
      {
        if (p1[0] != pm[0] || p1[1] != pm[1] || p1[2] != pm[2])
          new_color = (p1[2] / 51) + 6 * ((p1[1] / 51) + 6 * (p1[0] / 51));
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
    std::cerr << "- generate ppm" << std::endl;
    len = sprintf(buffer, "P6 %d %d 255\n", width, height);
    sep_file.fwrite(buffer, len);
    row1 = data1;
    for (int y = 0; y < height; y++)
    {
      sep_file.fwrite(row1, width * 3);
      row1 += row_size;
    }
    std::cerr << "- about to call csepdjvu" << std::endl;
    std::ostringstream csepdjvu_command;
    csepdjvu_command << "/usr/bin/csepdjvu";
    csepdjvu_command << " -d " << conf_dpi;
    if (conf_bg_slices)
      csepdjvu_command << " -q " << conf_bg_slices;
    csepdjvu_command << " " << sep_file.get_name() << " " << page_file.get_name();
    std::string csepdjvu_command_str = csepdjvu_command.str();
    xsystem(csepdjvu_command_str);
    djvm_command += " ";
    djvm_command += page_file.get_name();
    std::cerr << "- done!" << std::endl;
  }
  std::cerr << "About to call djvm" << std::endl;
  xsystem(djvm_command);
  std::cerr << "Done!" << std::endl;

  pass_to_stdout(output_file.get_name());
  
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
