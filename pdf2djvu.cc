#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>

#include "compoppler.h"

#include <libdjvu/miniexp.h>

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

  friend std::ostream &operator<<(std::ostream &, const Error &);
protected:
  std::string message;
};

std::ostream &operator<<(std::ostream &stream, const Error &error)
{
  return stream << error.message;
}

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
    <<  w << "x" <<  h << "+" << x << "+" << (y - h) << " "
    << "(";
  static char buffer[8];
  while (len > 0 && *unistr == ' ')
    unistr++, len--;
  if (len == 0)
    return std::string();
  for (; len >= 0; len--, unistr++)
  {
    if (*unistr < 0x20 || *unistr == ')' || *unistr == '\\')
      sprintf(buffer, "\\%03o", *unistr);
    else
    {
      int seqlen = mapUTF8(*unistr, buffer, sizeof buffer);
      buffer[seqlen] = '\0';
      strstream << buffer;
    }
  }
  strstream << ")" << std::endl;
  return strstream.str();
}

static void lisp_escape(std::string &value)
{
  miniexp_t exp = miniexp_string(value.c_str());
  exp = miniexp_pname(exp, 0);
  value = miniexp_to_str(exp);
}

class NoLinkDestination : public Error
{
public:
  NoLinkDestination() : Error("Cannot find link destination") {}
};

static int get_page_for_LinkGoTo(LinkGoTo *goto_link, Catalog *catalog)
{
  LinkDest *dest = goto_link->getDest();
  if (dest == NULL)
    dest = catalog->findDest(goto_link->getNamedDest());
  else
    dest = dest->copy();
  if (dest != NULL)
  {
    int page;
    if (dest->isPageRef())
    {
      Ref pageref = dest->getPageRef();
      page = catalog->findPage(pageref.num, pageref.gen);
    }
    else 
      page = dest->getPageNum();
    delete dest;
    return page;
  }
  else
    throw NoLinkDestination();
}

class MutedRenderer: public Renderer
{
private:
  std::vector<std::string> texts;
  std::vector<std::string> annotations;
public:
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y, CharCode code, int n_bytes, Unicode *unistr, int len)
  {
    texts.push_back(text_comment(
      (int) (x / 72 * conf_dpi), 
      (int) (getBitmapHeight() - y / 72 * conf_dpi),
      (int) (dx / 72 * conf_dpi),
      (int) (dy / 72 * conf_dpi),
      (int) (dx / 72 * conf_dpi),
      (int) (state->getFontSize() / 72 * conf_dpi),
      unistr,
      len
    ));
  }

  void drawLink(Link *link, Catalog *catalog)
  {
    double x1, y1, x2, y2;
    LinkAction *link_action = link->getAction();
    std::string uri;
    std::string border_color = get_link_border_color(link);
    link->getRect(&x1, &y1, &x2, &y2);
    switch (link_action->getKind())
    {
    case actionURI:
      uri += dynamic_cast<LinkURI*>(link_action)->getURI()->getCString();
      lisp_escape(uri);
      break;
    case actionGoTo:
    {
      int page;
      try
      {
        int page = get_page_for_LinkGoTo(dynamic_cast<LinkGoTo*>(link_action), catalog);
      }
      catch (NoLinkDestination &ex)
      {
        std::cerr << "[Warning] " << ex << std::endl;
      }
      std::ostringstream strstream;
      strstream << "\"#" << page << "\"";
      uri = strstream.str();
      break;
    }
    default:
      std::cerr << "[Warning] Unknown link type" << std::endl;
      return;
    }
    int x = (int) (x1 / 72 * conf_dpi);
    int y = (int) (y1 / 72 * conf_dpi);
    int w = (int) ((x2 - x1) / 72 * conf_dpi);
    int h = (int) ((y2 - y1) / 72 * conf_dpi);
    std::ostringstream strstream;
    strstream << "(maparea" 
      << " " << uri
      << " \"\"" 
      << " (rect " << x << " " << y << " " << w << " " << h << ")"
      << " (border " << border_color << ")"
      << ")";
    annotations.push_back(strstream.str());
  }

  GBool useDrawChar() { return gTrue; }

  MutedRenderer(SplashColor &paper_color) : Renderer(paper_color)
  { }

  std::vector<std::string> &get_annotations()
  {
    return annotations;
  }

  void clear_annotations()
  {
    annotations.clear();
  }

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
        catch (PagesParseError &ex)
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

static void xclose(int fd)
{
  if (close(fd) == -1)
    throw OSError();
}

class TemporaryFile : public std::fstream
{
public:
  TemporaryFile()
  {
    this->exceptions(std::ifstream::badbit);
    char file_name_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    int fd = mkstemp(file_name_buffer);
    if (fd == -1)
      throw OSError();
    xclose(fd);
    name = std::string(file_name_buffer);
    this->open(file_name_buffer, std::fstream::in | std::fstream::out | std::fstream::trunc);
  }

  ~TemporaryFile()
  {
    if (this->is_open())
      this->close();
    if (unlink(name.c_str()) == -1)
      throw OSError();
  }

  void pass(std::ostream &stream)
  {
    this->seekg(0, std::ios::beg);
    char buffer[BUFSIZ];
    while (! this->eof())
    {
      this->read(buffer, sizeof buffer);
      stream.write(buffer, this->gcount());
    }
  } 

  friend std::ostream &operator<<(std::ostream &, const TemporaryFile &);
  friend void operator+=(std::string &, const TemporaryFile &);

private:
  std::string name;
};

std::ostream &operator<<(std::ostream &out, const TemporaryFile &file)
{
  return out << file.name;
}

void operator+=(std::string &str, const TemporaryFile &file)
{
  str += file.name;
}

class NoPageForBookmark : public Error
{
public:
  NoPageForBookmark() : Error("No page for a bookmark") {}
};

class NoTitleForBookmark : public Error
{
public:
  NoTitleForBookmark() : Error("No title for a bookmark") {}
};

class IconvError : public Error
{
public:
  IconvError() : Error("Unable to convert encodings") {} 
};

std::string pdf_string_to_utf8_string(GooString *from)
{
  bool is_unicode = false;
  Unicode unicode;
  char *cfrom = from->getCString();
  std::ostringstream stream;
  if ((cfrom[0] & 0xff) == 0xfe && (cfrom[1] & 0xff) == 0xff)
  {
    static char outbuf[1 << 10];
    char *outbuf_ptr = outbuf;
    size_t outbuf_len = sizeof outbuf;
    size_t inbuf_len = strlen(cfrom);
    cfrom += 2;
    iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
    if (cd == (iconv_t)-1)
      throw OSError();
    while (inbuf_len > 0)
    {
      size_t n = iconv(cd, &cfrom, &inbuf_len, &outbuf_ptr, &outbuf_len);
      if (n == -1 && errno == E2BIG)
      {
        stream.write(outbuf, outbuf_ptr - outbuf);
        outbuf_ptr = outbuf;
        outbuf_len = sizeof outbuf;
      }
      else if (n == -1)
        throw IconvError();
    }
    stream.write(outbuf, outbuf_ptr - outbuf);
    if (iconv_close(cd) == -1)
      throw OSError();
  }
  else
  {
    for (; *cfrom; cfrom++)
    {
      static char buffer[8];
      Unicode unichr = pdfDocEncoding[*cfrom & 0xff];
      int seqlen = mapUTF8(unichr, buffer, sizeof buffer);
      buffer[seqlen] = 0;
      stream.write(buffer, seqlen);
    }
  }
  return stream.str();
}

void pdf_outline_to_djvu_outline(Object *node, Catalog *catalog, std::ostream &stream)
{
  Object current, next;
  if (!dict_lookup(node, "First", &current)->isDict())
    return;
  while (current.isDict())
  {
    Object title;
    if (!dict_lookup(current, "Title", &title)->isString())
      throw NoTitleForBookmark();
    std::string title_str = pdf_string_to_utf8_string(title.getString());
    title.free();

    Object destination;
    LinkAction *link_action;
    int page;
    if (!dict_lookup(current, "Dest", &destination)->isNull())
      link_action = LinkAction::parseDest(&destination);
    else if (!dict_lookup(current, "A", &destination)->isNull())
      link_action = LinkAction::parseAction(&destination);
    else
      throw NoPageForBookmark();
    if (link_action->getKind() != actionGoTo)
      throw NoPageForBookmark();
    try
    {
      page = get_page_for_LinkGoTo(dynamic_cast<LinkGoTo*>(link_action), catalog);
    }
    catch (NoLinkDestination &ex)
    {
      std::cerr << "[Warning] " << ex << std::endl;
      page = -1;
    }
    destination.free();
   
    if (page >= 0)
    {
      lisp_escape(title_str);
      stream << "(" << title_str << " \"#" << page << "\"";
      pdf_outline_to_djvu_outline(&current, catalog, stream);
      stream << ") ";
    }

    dict_lookup(current, "Next", &next);
    current.free();
    current = next;
  }
  current.free();
}

void pdf_outline_to_djvu_outline(PDFDoc *doc, std::ostream &stream)
{
  Catalog *catalog = doc->getCatalog();
  Object *outlines = catalog->getOutline();
  if (!outlines->isDict())
    return;
  stream << "(bookmarks ";
  pdf_outline_to_djvu_outline(outlines, catalog, stream);
  stream << ")";
}

class InvalidDateFormat : public Error { };

void pdf_metadata_to_djvu_metadata(PDFDoc *doc, std::ostream &stream)
{
  static const char* string_keys[] = { "Title", "Subject", "Keywords", "Author", "Creator", "Producer", NULL };
  static const char* date_keys[] = { "CreationDate", "ModDate", NULL };
  Object info;
  doc->getDocInfo(&info);
  if (!info.isDict())
    return;
  Dict *info_dict = info.getDict();
  for (const char** pkey = string_keys; *pkey; pkey++)
  {
    Object object;
    if (!dict_lookup(info_dict, *pkey, &object)->isString())
      continue;
    std::string value = pdf_string_to_utf8_string(object.getString());
    lisp_escape(value);
    stream << *pkey << "\t" << value << std::endl;
  }
  for (const char** pkey = date_keys; *pkey; pkey++)
  try
  {
    Object object;
    struct tm tms;
    char tzs; int tz1, tz2;
    char buffer[32], tzbuffer[8];
    if (!dict_lookup(info_dict, *pkey, &object)->isString())
      continue;
    char *date_str = object.getString()->getCString();
    if (date_str[0] == 'D' && date_str[1] == ':')
      date_str += 2;
    if (sscanf(date_str, "%4d%2d%2d%2d%2d%2d%c%2d'%2d'", &tms.tm_year, &tms.tm_mon, &tms.tm_mday, &tms.tm_hour, &tms.tm_min, &tms.tm_sec, &tzs, &tz1, &tz2) != 9)
      throw InvalidDateFormat();
    tms.tm_year -= 1900;
    tms.tm_mon -= 1;
    tms.tm_wday = tms.tm_yday = tms.tm_isdst = -1;
    if (mktime(&tms) == (time_t)-1)
      throw InvalidDateFormat();
    // RFC 3339 date format, e.g. "2007-10-27 13:19:59+02:00"
    if (strftime(buffer, sizeof buffer, "%F %T", &tms) != 19)
      throw InvalidDateFormat();
    if ((tzs != '+' && tzs != '-') || tz1 < 0 || tz1 > 12 || tz2 >= 60 || tz2 < 0)
      throw InvalidDateFormat();
    if (snprintf(tzbuffer, sizeof tzbuffer, "%c%02d:%02d", tzs, tz1, tz2) != 6)
      throw InvalidDateFormat();
    stream << *pkey << "\t\"" << buffer << tzbuffer << "\"" << std::endl;
  }
  catch (InvalidDateFormat &ex)
  {
    std::cerr << "[Warning] metadata[" << *pkey << "] is not a valid date" << std::endl;
  }
}

static int xmain(int argc, char **argv)
{
  if (!read_config(argc, argv))
    usage();

  init_global_params();
  if (!set_antialias(conf_antialias))
    throw Error();

  PDFDoc *doc = new_document(file_name);
  if (!doc->isOk())
    throw Error("Unable to load document");
  
  std::cerr << doc->getFileName()->getCString() << ":" << std::endl;

  SplashColor paper_color;
  set_color(paper_color, 0xff, 0xff, 0xff);

  Renderer *out1 = new Renderer(paper_color);
  MutedRenderer *outm = new MutedRenderer(paper_color);
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
    std::cerr << "- page #" << n << ":" << std::endl;
    std::cerr << "  - render with text" << std::endl;
    display_page(doc, out1, n, conf_dpi, false);
    std::cerr << "  - render without text" << std::endl;
    display_page(doc, outm, n, conf_dpi, true);
    std::cerr << "  - take bitmaps" << std::endl;
    Pixmap *bmp1 = new Pixmap(out1), *bmpm = new Pixmap(outm);
    PixmapIterator p1 = bmp1->begin();
    PixmapIterator pm = bmpm->begin();
    int width = bmp1->get_width();
    int height = bmp1->get_height();
    std::cerr << "  - create sep_file" << std::endl;
    TemporaryFile sep_file;
    sep_file << "R6 " << width << " " << height << " 216" << std::endl;
    std::cerr << "  - rle palette >> sep_file" << std::endl;
    for (int r = 0; r < 6; r++)
    for (int g = 0; g < 6; g++)
    for (int b = 0; b < 6; b++)
    {
      char buffer[] = { 51 * r, 51 * g, 51 * b };
      sep_file.write(buffer, 3);
    }
    std::cerr << "  - rle data >> sep_file" << std::endl;
    bool has_background = false;
    bool has_foreground = false;
    bool has_text = false;
    for (int y = 0; y < height; y++)
    {
      int new_color, color = 0xfff;
      int length = 0;
      for (int x = 0; x < width; x++)
      {
        if (!has_background && (pm[0] & pm[0] & pm[0] & 0xff) != 0xff)
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
            sep_file.write(&c, 1);
          }
          color = new_color;
          length = 1;
        }
        p1++, pm++;
      }
      p1.next_row(), pm.next_row();
      int item = (color << 20) + length;
      for (int i = 0; i < 4; i++)
      {
        char c = item >> ((3 - i) * 8);
        sep_file.write(&c, 1);
      }
    }
    delete bmpm;
    if (has_background)
    {
      std::cerr << "  - background pixmap >> sep_file" << std::endl;
      sep_file << "P6 " << width << " " << height << " 255" << std::endl;
      sep_file << *bmp1;
    }
    delete bmp1;
    {
      std::cerr << "  - text layer >> sep_file" << std::endl;
      std::vector<std::string> &texts = outm->get_texts();
      for (std::vector<std::string>::iterator it = texts.begin(); it != texts.end(); it++)
      {
        if (it->size() == 0)
          continue;
        sep_file << *it;
        has_text = true;
      }
      outm->clear_texts();
    }
    std::cerr << "  - !csepdjvu" << std::endl;
    std::ostringstream csepdjvu_command;
    csepdjvu_command << "/usr/bin/csepdjvu";
    csepdjvu_command << " -d " << conf_dpi;
    if (conf_bg_slices)
      csepdjvu_command << " -q " << conf_bg_slices;
    sep_file.close();
    csepdjvu_command << " " << sep_file << " " << page_file;
    std::string csepdjvu_command_str = csepdjvu_command.str();
    xsystem(csepdjvu_command_str);
    djvm_command += " ";
    djvm_command += page_file;
    /* XXX csepdjvu produces ridiculously large Sjbz chunks. */
    TemporaryFile rle_file, sjbz_file, fgbz_file, bg44_file, sed_file;
    {
      std::cerr << "  - !ddjvu" << std::endl;
      std::ostringstream command;
      command << "/usr/bin/ddjvu -format=rle -mode=mask " << page_file << " " << rle_file;
      xsystem(command);
    }
    if (has_background || has_foreground)
    { 
      std::cerr << "  - !djvuextract" << std::endl;
      std::ostringstream command;
      command << "/usr/bin/djvuextract " << page_file << " FGbz=" << fgbz_file << " BG44=" << bg44_file;
      xsystem(command);
    }
    {
      std::cerr << "  - annotations >> sed_file" << std::endl;
      std::vector<std::string> &annotations = outm->get_annotations();
      sed_file << "select 1" << std::endl << "set-ant" << std::endl;
      for (std::vector<std::string>::iterator it = annotations.begin(); it != annotations.end(); it++)
        sed_file << *it << std::endl;
      sed_file << "." << std::endl;
      outm->clear_annotations();
    }
    if (has_text)
    {
      std::cerr << "  - !djvused >> sed_file" << std::endl;
      std::ostringstream command;
      command << "/usr/bin/djvused " << page_file << " -e output-txt >> " << sed_file;
      xsystem(command);
    }
    {
      std::cerr << "  - !cjb2" << std::endl;
      std::ostringstream command;
      command << "/usr/bin/cjb2 " << rle_file << " " << sjbz_file;
      xsystem(command);
    }
    {
      std::cerr << "  - !djvumake" << std::endl;
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
    {
      std::cerr << "  - !djvused < sed_file" << std::endl;
      std::ostringstream command;
      command << "/usr/bin/djvused " << page_file << " -s -f " << sed_file;
      xsystem(command);
    }
  }
  std::cerr << "- !djvm" << std::endl;
  xsystem(djvm_command);
  {
    TemporaryFile sed_file;
    {
      std::cerr << "- outlines >> sed_file" << std::endl;
      sed_file << "set-outline" << std::endl;
      pdf_outline_to_djvu_outline(doc, sed_file);
      sed_file << std::endl << "." << std::endl;
    }
    {
      std::cerr << "- metadata >> sed_file" << std::endl;
      sed_file << "set-meta" << std::endl;
      pdf_metadata_to_djvu_metadata(doc, sed_file);
      sed_file << "." << std::endl;
    }
    std::cerr << "- !djvused < sed_file" << std::endl;
    std::ostringstream command;
    sed_file.close();
    command << "/usr/bin/djvused " << output_file << " -s -f " << sed_file;
    xsystem(command);
  }
  output_file.pass(std::cout);
  
  delete[] page_files;

  return 0;
}

int main(int argc, char **argv)
{
  try
  {
    xmain(argc, argv);
  }
  catch (Error &ex)
  {
    std::cerr << ex << std::endl;
    exit(1);
  }
}

// vim:ts=2 sw=2 et
