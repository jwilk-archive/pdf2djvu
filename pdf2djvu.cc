/* Copyright © 2007 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef DJVULIBRE_BIN_PATH
#error You need to define DJVULIBRE_BIN_PATH
#endif

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <stdexcept>

#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <iconv.h>

#include "compoppler.h"

#include <pstreams/pstream.h>
#include <libdjvu/miniexp.h>

static enum
{
  CONF_TEXT_NONE = 0,
  CONF_TEXT_WORDS,
  CONF_TEXT_LINES
} conf_text = CONF_TEXT_WORDS;
static enum
{
  CONF_FORMAT_BUNDLED,
  CONF_FORMAT_INDIRECT
} conf_format = CONF_FORMAT_BUNDLED;
static std::string conf_output;
static bool conf_output_stdout = true;
static int conf_verbose = 1;
static int conf_dpi = 300;
static int conf_bg_subsample = 3;
static bool conf_antialias = false;
static bool conf_extract_hyperlinks = true;
static bool conf_extract_metadata = true;
static bool conf_extract_outline = true;
static bool conf_no_render = false;
static char *conf_bg_slices = NULL;
static std::vector< std::pair<int, int> > conf_pages;
static char *file_name;

class DevNull : public std::ostream
{
public:
  DevNull() : std::ostream(0) { }
};

static DevNull dev_null;

static std::ostream &debug(int n)
{
  if (n <= conf_verbose)
    return std::clog;
  else
    return dev_null;
}

class Error
{
public:
  Error() : message("Unknown error") {};
  explicit Error(const std::string &message) : message(message) {};
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

class ConfigurationError : public Error
{
public:
  explicit ConfigurationError(const std::string &message) : Error(message) {};
  virtual bool is_quiet() const
  {
    return false;
  }
  virtual bool is_already_printed() const
  {
    return false;
  }
  virtual ~ConfigurationError() { /* just to shut up compilers */ }
};

class PagesParseError : public ConfigurationError
{
public:
  PagesParseError() : ConfigurationError("Unable to parse page numbers") {}
};

class DpiOutsideRange : public ConfigurationError
{
public:
  DpiOutsideRange(int dpi_from, int dpi_to) : ConfigurationError("The specified resolution is outside the allowed range")
  {
    std::ostringstream stream;
    stream << ": " << dpi_from << " .. " << dpi_to;
    this->message += stream.str();
  }
};

class NeedHelp : public ConfigurationError
{
public:
  NeedHelp() : ConfigurationError("") {};
  virtual bool is_quiet() const
  {
    return true;
  }
};

class InvalidOption : public ConfigurationError
{
public:
  InvalidOption() : ConfigurationError("") {};
  virtual bool is_quiet() const
  {
    return true;
  }
  virtual bool is_already_printed() const
  {
    return true;
  }
};

static std::string text_comment(int x, int y, int dx, int dy, int w, int h, const Unicode *unistr, int len)
{
  std::ostringstream strstream;
  strstream
    << "# T " 
    <<  x << ":" <<  y << " " 
    << dx << ":" << dy << " "
    <<  w << "x" <<  h << std::showpos << x << (y - h) << " "
    << "(";
  while (len > 0 && *unistr == ' ')
    unistr++, len--;
  if (len == 0)
    return std::string();
  for (; len >= 0; len--, unistr++)
  {
    if (*unistr < 0x20 || *unistr == ')' || *unistr == '\\')
      strstream << "\\" << std::oct << std::setfill('0') << std::setw(3) << static_cast<unsigned int>(*unistr);
    else
    {
      char buffer[8];
      int seqlen = mapUTF8(*unistr, buffer, sizeof buffer);
      strstream.write(buffer, seqlen);
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
  std::auto_ptr<LinkDest> dest_copy;
  LinkDest *dest = goto_link->getDest();
  if (dest == NULL)
    dest = catalog->findDest(goto_link->getNamedDest());
  else
  {
    dest = dest->copy();
    dest_copy.reset(dest);
  }
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
    return page;
  }
  else
    throw NoLinkDestination();
}

static bool is_foreground_color_map(GfxImageColorMap *color_map)
{
  return (color_map->getNumPixelComps() <= 1 && color_map->getBits() <= 1);
}

class MutedRenderer: public Renderer
{
private:
  std::vector<std::string> texts;
  std::vector<std::string> annotations;
  std::map<int, int> &page_map;
public:

  void drawImageMask(GfxState *state, Object *object, Stream *stream, int width, int height, GBool invert, GBool inline_image)
  {
    return;
  }

  void drawImage(GfxState *state, Object *object, Stream *stream, int width, int height, GfxImageColorMap *color_map, int *mask_colors, GBool inline_image)
  {
    if (is_foreground_color_map(color_map))
      return;
    Renderer::drawImage(state, object, stream, width, height, color_map, mask_colors, inline_image);
  }

  void drawMaskedImage(GfxState *state, Object *object, Stream *stream, int width, int height, GfxImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height, GBool mask_invert)
  {
    if (is_foreground_color_map(color_map))
      return;
    Renderer::drawMaskedImage(state, object, stream, width, height, color_map, mask_stream, mask_width, mask_height, mask_invert);
  }

  void drawSoftMaskedImage(GfxState *state, Object *object, Stream *stream, int width, int height, GfxImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height,	GfxImageColorMap *mask_color_map)
  {
    if (is_foreground_color_map(color_map))
      return;
    Renderer::drawSoftMaskedImage(state, object, stream, width, height, color_map, mask_stream, mask_width, mask_height, mask_color_map);
  }

  GBool interpretType3Chars() { return gFalse; }

  void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y, CharCode code, int n_bytes, Unicode *unistr, int len)
  {
    double px, py, pdx, pdy;
    state->transform(x, y, &px, &py);
    state->transformDelta(dx, dy, &pdx, &pdy);
    int old_render = state->getRender();
    state->setRender(0x103);
    this->Renderer::drawChar(state, x, y, dx, dy, origin_x, origin_y, code, n_bytes, unistr, len);
    state->setRender(old_render);
    int font_size = static_cast<int>(state->getTransformedFontSize());
    SplashFont *font = this->getCurrentFont();
    SplashGlyphBitmap glyph;
    if (get_glyph(this->getSplash(), font, code, &glyph))
    {
      font_size = glyph.h;
      py += glyph.h - glyph.y;
    }
    texts.push_back(text_comment(
      static_cast<int>(px),
      static_cast<int>(py),
      static_cast<int>(pdx),
      static_cast<int>(pdy),
      static_cast<int>(pdx < 1 ? 1 : pdx),
      font_size,
      unistr,
      len
    ));
  }

  void drawLink(Link *link, Catalog *catalog)
  {
    if (!conf_extract_hyperlinks)
      return;
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
        page = get_page_for_LinkGoTo(dynamic_cast<LinkGoTo*>(link_action), catalog);
      }
      catch (NoLinkDestination &ex)
      {
        debug(1) << "[Warning] " << ex << std::endl;
        return;
      }
      std::ostringstream strstream;
      strstream << "\"#" << this->page_map[page] << "\"";
      uri = strstream.str();
      break;
    }
    case actionGoToR:
      debug(1) << "[Warning] Unable to convert link with a remote go-to action" << std::endl;
      return;
    case actionNamed:
      debug(1) << "[Warning] Unable to convert link with a named action" << std::endl;
      return;
    default:
      debug(1) << "[Warning] Unknown link action" << std::endl;
      return;
    }
    int x, y, w, h;
    this->cvtUserToDev(x1, y1, &x, &y);
    this->cvtUserToDev(x2, y2, &w, &h);
    w -= x;
    h = y - h;
    y = this->getBitmapHeight() - y;
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

  void stroke(GfxState *state) { }
  void fill(GfxState *state)
  { 
    SplashPath path;
    this->convert_path(state, path);
    double area = get_path_area(path);
    if (area / this->getBitmapHeight() / this->getBitmapWidth() >= 0.8)
      Renderer::fill(state);
  }
  void eoFill(GfxState *state) { }

  MutedRenderer(SplashColor &paper_color, std::map<int, int> &page_map) : Renderer(paper_color), page_map(page_map)
  { }

  const std::vector<std::string> &get_annotations() const
  {
    return annotations;
  }

  void clear_annotations()
  {
    annotations.clear();
  }

  const std::vector<std::string> &get_texts() const
  {
    return texts;
  }

  void clear_texts()
  {
    texts.clear();
  }
};

static void usage(const ConfigurationError &error)
{
  if (error.is_already_printed())
    debug(0) << std::endl;
  if (!error.is_quiet())
    debug(0) << error << std::endl << std::endl;
  debug(0) 
    << "Usage: " << std::endl
    << "   pdf2djvu [-o <output-djvu-file>] [options] <pdf-file>" << std::endl
    << "   pdf2djvu  -i <output-directory>  [options] <pdf-file>" << std::endl
    << std::endl
    << "Options:" << std::endl
    << " -i, --indirect=DIR"      << std::endl
    << " -o, --output=FILE"       << std::endl
    << " -v, --verbose"           << std::endl
    << " -q, --quiet"             << std::endl
    << " -d, --dpi=resolution"    << std::endl
    << "     --bg-slices=n,...,n" << std::endl
    << "     --bg-slices=n+...+n" << std::endl
    << "     --bg-subsample=n"    << std::endl
    << "     --antialias"         << std::endl
    << "     --no-metadata"       << std::endl
    << "     --no-outline"        << std::endl
    << "     --no-hyperlinks"     << std::endl
    << "     --no-text"           << std::endl
    << "     --words"             << std::endl
    << "     --lines"             << std::endl
    << " -p, --pages=..."         << std::endl
    << " -h, --help"              << std::endl
  ;
  exit(1);
}

static void parse_pages(const std::string &s, std::vector< std::pair<int, int> > &result)
{
  int state = 0;
  int value[2] = { 0, 0 };
  for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
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

/* XXX
 * csepdjvu requires 25 <= dpi <= 144 000
 * djvumake requires 72 <= dpi <= 144 000
 * cpaldjvu requires 25 <= dpi <=   1 200 (but we don't use it)
 */
static const int DJVU_MIN_DPI = 72;
static const int DJVU_MAX_DPI = 144000;

static void read_config(int argc, char * const argv[])
{
  enum
  {
    OPT_ANTIALIAS    = 0x300,
    OPT_BG_SLICES    = 0x200,
    OPT_BG_SUBSAMPLE = 0x201,
    OPT_DPI          = 'd',
    OPT_HELP         = 'h',
    OPT_INDIRECT     = 'i',
    OPT_NO_HLINKS    = 0x401,
    OPT_NO_METADATA  = 0x402,
    OPT_NO_OUTLINE   = 0x403,
    OPT_NO_RENDER    = 0x400,
    OPT_OUTPUT       = 'o',
    OPT_PAGES        = 'p',
    OPT_QUIET        = 'q',
    OPT_TEXT_LINES   = 0x102,
    OPT_TEXT_NONE    = 0x100,
    OPT_TEXT_WORDS   = 0x101,
    OPT_VERBOSE      = 'v'
  };
  static struct option options [] =
  {
    { "dpi",            1, 0, OPT_DPI },
    { "quiet",          0, 0, OPT_QUIET },
    { "verbose",        0, 0, OPT_VERBOSE },
    { "bg-slices",      1, 0, OPT_BG_SLICES },
    { "bg-subsample",   1, 0, OPT_BG_SUBSAMPLE },
    { "antialias",      0, 0, OPT_ANTIALIAS },
    { "no-hyperlinks",  0, 0, OPT_NO_HLINKS },
    { "no-metadata",    0, 0, OPT_NO_METADATA },
    { "no-outline",     0, 0, OPT_NO_OUTLINE },
    { "no-render",      0, 0, OPT_NO_RENDER },
    { "pages",          1, 0, OPT_PAGES },
    { "help",           0, 0, OPT_HELP },
    { "no-text",        0, 0, OPT_TEXT_NONE },
    { "words",          0, 0, OPT_TEXT_WORDS },
    { "lines",          0, 0, OPT_TEXT_LINES },
    { "output",         1, 0, OPT_OUTPUT },
    { "indirect",       0, 0, OPT_INDIRECT },
    { NULL,             0, 0, '\0' }
  };
  int optindex, c;
  while (true)
  {
    optindex = 0;
    c = getopt_long(argc, argv, "i:o:d:qvp:h", options, &optindex);
    if (c < 0)
      break;
    if (c == 0)
      throw Error("");
    switch (c)
    {
    case OPT_DPI:
      conf_dpi = atoi(optarg);
      if (conf_dpi < DJVU_MIN_DPI || conf_dpi > DJVU_MAX_DPI)
        throw DpiOutsideRange(DJVU_MIN_DPI, DJVU_MAX_DPI);
      break;
    case OPT_QUIET:
      conf_verbose = 0;
      break;
    case OPT_VERBOSE:
      conf_verbose = 2;
      break;
    case OPT_BG_SLICES:
      conf_bg_slices = optarg;
      break;
    case OPT_BG_SUBSAMPLE:
      conf_bg_subsample = atoi(optarg);
      if (conf_bg_subsample < 1 || conf_bg_subsample > 11)
        throw ConfigurationError("The specified subsampling ratio is outside the allowed range");
      break;
    case OPT_PAGES:
      parse_pages(optarg, conf_pages);
      break;
    case OPT_ANTIALIAS:
      conf_antialias = 1;
      break;
    case OPT_NO_HLINKS:
      conf_extract_hyperlinks = false;
      break;
    case OPT_NO_METADATA:
      conf_extract_metadata = false;
      break;
    case OPT_NO_OUTLINE:
      conf_extract_outline = false;
      break;
    case OPT_NO_RENDER:
      conf_no_render = 1;
      break;
    case OPT_TEXT_NONE:
      conf_text = CONF_TEXT_NONE;
      break;
    case OPT_TEXT_WORDS:
      conf_text = CONF_TEXT_WORDS;
      break;
    case OPT_TEXT_LINES:
      conf_text = CONF_TEXT_LINES;
      break;
    case OPT_OUTPUT:
      conf_format = CONF_FORMAT_BUNDLED;
      conf_output = optarg;
      conf_output_stdout = false;
      break;
    case OPT_INDIRECT:
      conf_format = CONF_FORMAT_INDIRECT;
      conf_output = optarg;
      conf_output_stdout = false;
      break;
    case OPT_HELP:
      throw NeedHelp();
    case '?':
    case ':':
      throw InvalidOption();
    default:
      throw std::logic_error("Unknown option");
    }
  }
  if (optind < argc - 1)
    throw ConfigurationError("Too many arguments were specified");
  else if (optind > argc - 1)
    throw ConfigurationError("No input file name was specified");
  else
    file_name = argv[optind];
}

static void copy_stream(std::istream &istream, std::ostream &ostream, bool seek)
{
  if (seek)
    istream.seekg(0, std::ios::beg);
  char buffer[BUFSIZ];
  while (!istream.eof())
  {
    istream.read(buffer, sizeof buffer);
    ostream.write(buffer, istream.gcount());
  }
}

static void copy_stream(std::istream &istream, std::ostream &ostream, bool seek, std::streamsize limit)
{
  if (seek)
    istream.seekg(0, std::ios::beg);
  char buffer[BUFSIZ];
  while (!istream.eof() && limit > 0)
  {
    std::streamsize chunk_size = std::min(static_cast<std::streamsize>(sizeof buffer), limit);
    istream.read(buffer, chunk_size);
    ostream.write(buffer, istream.gcount());
    limit -= chunk_size;
  }
}

class Command
{
private:
  std::string command;
  redi::pstreams::argv_type argv;

  void call(std::ostream *my_stdout, bool quiet = false)
  {
    redi::ipstream xsystem(this->command, this->argv, redi::pstream::pstdout | redi::pstream::pstderr);
    if (my_stdout != NULL)
    {
      std::string stdout_line;
      xsystem.out();
      copy_stream(xsystem, *my_stdout, false);
    }
    {
      std::string stderr_line;
      xsystem.err();
      copy_stream(xsystem, quiet ? dev_null : std::cerr, false);
    }
    xsystem.close();
    int status = xsystem.rdbuf()->status();
    if (status != 0)
    {
      std::ostringstream message;
      message << "system(\"";
      message << this->command;
      message << " ...\") failed";
      if (WIFEXITED(status))
        message << " with exit code " << WEXITSTATUS(status);
      throw Error(message.str());
    }
  }

public:
  explicit Command(const std::string& command) : command(command)
  {
    this->argv.push_back(command);
  }

  Command &operator <<(const std::string& arg)
  {
    this->argv.push_back(arg);
    return *this;
  }

  Command &operator <<(int i)
  {
    std::ostringstream stream;
    stream << i;
    return *this << stream.str();
  }

  void operator()(std::ostream &my_stdout, bool quiet = false)
  {
    this->call(&my_stdout, quiet);
  }

  void operator()(bool quiet = false)
  {
    this->call(NULL, quiet);
  }
};

static void xclose(int fd)
{
  if (close(fd) == -1)
    throw OSError();
}

class Directory
{
protected:
  std::string name;
public: 
  explicit Directory(const std::string &name)
  : name(name) 
  { }
  virtual ~Directory() {}
  friend std::ostream &operator<<(std::ostream &, const Directory &);
};

class TemporaryDirectory : public Directory
{
private:
  TemporaryDirectory(const TemporaryDirectory&); // not defined
  TemporaryDirectory& operator=(const TemporaryDirectory&); // not defined
public:
  TemporaryDirectory() : Directory("")
  {
    char path_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    if (mkdtemp(path_buffer) == NULL)
      throw OSError();
    this->name += path_buffer;
  }

  virtual ~TemporaryDirectory()
  {
    if (rmdir(this->name.c_str()) == -1)
      throw OSError();
  }
};

std::ostream &operator<<(std::ostream &out, const Directory &directory)
{
  return out << directory.name;
}

class File : public std::fstream
{
protected:
  std::string name;

  void _open(const char* path)
  {
    this->exceptions(std::ifstream::failbit | std::ifstream::badbit);
    if (path == NULL)
      this->open(this->name.c_str(), std::fstream::in | std::fstream::out);
    else
    {
      this->name = path;
      this->open(path, std::fstream::in | std::fstream::out | std::fstream::trunc);
    }
    this->exceptions(std::ifstream::badbit);
  }

  File() {}

public:
  explicit File(const std::string &name)
  {
    _open(name.c_str());
  }

  File(const Directory& directory, const std::string &name)
  {
    std::ostringstream stream;
    stream << directory << "/" << name;
    _open(stream.str().c_str());
  }

  virtual ~File() { }

  void reopen()
  {
    if (this->is_open())
      this->close();
    this->_open(NULL);
  }

  operator const std::string& () const
  {
    return this->name;
  }

  friend std::ostream &operator<<(std::ostream &, const File &);
};

class TemporaryFile : public File
{
private:
  TemporaryFile(const TemporaryFile&); // not defined
  TemporaryFile& operator=(const TemporaryFile&); // not defined
protected:
  void construct()
  {
    char path_buffer[] = "/tmp/pdf2djvu.XXXXXX";
    int fd = mkstemp(path_buffer);
    if (fd == -1)
      throw OSError();
    xclose(fd);
    _open(path_buffer);
  }

public:
  TemporaryFile(const Directory& directory, const std::string &name) 
  : File(directory, name) 
  { }

  TemporaryFile()
  {
    this->construct();
  }

  virtual ~TemporaryFile()
  {
    if (this->is_open())
      this->close();
    if (unlink(this->name.c_str()) == -1)
      throw OSError();
  }
};

std::ostream &operator<<(std::ostream &out, const File &file)
{
  return out << file.name;
}

class BookmarkError : public Error
{
public:
  BookmarkError(const std::string &message) : Error(message) {}
};

class NoPageForBookmark : public BookmarkError
{
public:
  NoPageForBookmark() : BookmarkError("No page for a bookmark") {}
};

class NoTitleForBookmark : public BookmarkError
{
public:
  NoTitleForBookmark() : BookmarkError("No title for a bookmark") {}
};

class IconvError : public Error
{
public:
  IconvError() : Error("Unable to convert encodings") {} 
};

static std::string pdf_string_to_utf8_string(GooString *from)
{
  char *cfrom = from->getCString();
  std::ostringstream stream;
  if ((cfrom[0] & 0xff) == 0xfe && (cfrom[1] & 0xff) == 0xff)
  {
    static char outbuf[1 << 10];
    char *outbuf_ptr = outbuf;
    size_t outbuf_len = sizeof outbuf;
    size_t inbuf_len = from->getLength();
    iconv_t cd = iconv_open("UTF-8", "UTF-16");
    if (cd == (iconv_t)-1)
      throw OSError();
    while (inbuf_len > 0)
    {
      struct iconv_adapter 
      {
        // http://wang.yuxuan.org/blog/2007/7/9/deal_with_2_versions_of_iconv_h
        iconv_adapter(const char** s) : s(s) {}
        iconv_adapter(char** s) : s(const_cast<const char**>(s)) {}
        operator char**() const
        {
          return const_cast<char**>(s);
        }
        operator const char**() const
        {
          return const_cast<const char**>(s);
        }
        const char** s;
      };

      size_t n = iconv(cd, iconv_adapter(&cfrom), &inbuf_len, &outbuf_ptr, &outbuf_len);
      if (n == (size_t) -1 && errno == E2BIG)
      {
        stream.write(outbuf, outbuf_ptr - outbuf);
        outbuf_ptr = outbuf;
        outbuf_len = sizeof outbuf;
      }
      else if (n == (size_t) -1)
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
      char buffer[8];
      Unicode unichr = pdfDocEncoding[*cfrom & 0xff];
      int seqlen = mapUTF8(unichr, buffer, sizeof buffer);
      buffer[seqlen] = 0;
      stream.write(buffer, seqlen);
    }
  }
  return stream.str();
}

static void pdf_outline_to_djvu_outline(Object *node, Catalog *catalog, std::ostream &stream, std::map<int, int> &page_map)
{
  Object current, next;
  if (!dict_lookup(node, "First", &current)->isDict())
    return;
  while (current.isDict())
  {
    try
    {
      std::string title_str;
      {
        XObject title;
        if (!dict_lookup(current, "Title", &title)->isString())
          throw NoTitleForBookmark();
        title_str = pdf_string_to_utf8_string(title.getString());
      } 

      int page;
      {
        XObject destination;
        LinkAction *link_action;
        if (!dict_lookup(current, "Dest", &destination)->isNull())
          link_action = LinkAction::parseDest(&destination);
        else if (!dict_lookup(current, "A", &destination)->isNull())
          link_action = LinkAction::parseAction(&destination);
        else
          throw NoPageForBookmark();
        if (!link_action || link_action->getKind() != actionGoTo)
          throw NoPageForBookmark();
        page = get_page_for_LinkGoTo(dynamic_cast<LinkGoTo*>(link_action), catalog);
      }

      lisp_escape(title_str);
      stream << "(" << title_str << " \"#" << page_map[page] << "\"";
      pdf_outline_to_djvu_outline(&current, catalog, stream, page_map);
      stream << ") ";
    }
    catch (BookmarkError &ex)
    {
      debug(1) << "[Warning] " << ex << std::endl;
    }

    dict_lookup(current, "Next", &next);
    current.free();
    current = next;
  }
  current.free();
}

static void pdf_outline_to_djvu_outline(PDFDoc *doc, std::ostream &stream, std::map<int, int> &page_map)
{
  Catalog *catalog = doc->getCatalog();
  Object *outlines = catalog->getOutline();
  if (!outlines->isDict())
    return;
  stream << "(bookmarks ";
  pdf_outline_to_djvu_outline(outlines, catalog, stream, page_map);
  stream << ")";
}

class InvalidDateFormat : public Error { };

static int scan_date_digits(char * &input, int n)
{
  int value = 0;
  for (int i = 0; i < n; i++)
  {
    if (*input >= '0' && *input <= '9')
    {
      value *= 10;
      value += *input - '0';
    }
    else
      throw InvalidDateFormat();
    input++;
  }
  return value;
}

static void pdf_metadata_to_djvu_metadata(PDFDoc *doc, std::ostream &stream)
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
    try
    {
      std::string value = pdf_string_to_utf8_string(object.getString());
      lisp_escape(value);
      stream << *pkey << "\t" << value << std::endl;
    }
    catch (IconvError &ex)
    {
      debug(1) << "[Warning] metadata[" << *pkey << "] is not properly encoded" << std::endl;
      continue;
    }
  }
  for (const char** pkey = date_keys; *pkey; pkey++)
  try
  {
    Object object;
    struct tm tms;
    char tzs; int tzh = 0, tzm = 0;
    char buffer[32];
    if (!dict_lookup(info_dict, *pkey, &object)->isString())
      continue;
    char *input = object.getString()->getCString();
    if (input[0] == 'D' && input[1] == ':')
      input += 2;
    tms.tm_year = scan_date_digits(input, 4) - 1900;
    tms.tm_mon = (*input ? scan_date_digits(input, 2) : 1) - 1;
    tms.tm_mday = *input ? scan_date_digits(input, 2) : 1;
    tms.tm_hour = *input ? scan_date_digits(input, 2) : 0;
    tms.tm_min  = *input ? scan_date_digits(input, 2) : 0;
    tms.tm_sec  = *input ? scan_date_digits(input, 2) : 0;
    switch (*input)
    {
    case '\0':
      tzs = 0;
      break;
    case '-':
    case '+':
      tzs = *input;
      input++;
      tzh = scan_date_digits(input, 2);
      if (tzh > 23)
        throw InvalidDateFormat();
      if (*input == '\'')
        input++;
      else
        throw InvalidDateFormat();
      tzm = scan_date_digits(input, 2);
      if (tzm > 59)
        throw InvalidDateFormat();
      if (*input == '\'')
        input++;
      else
        throw InvalidDateFormat();
      break;
    case 'Z':
      input++;
      tzs = '+';
      tzh = tzm = 0;
      break;
    default:
      throw InvalidDateFormat();
    }
    if (*input)
      throw InvalidDateFormat();
    if (mktime(&tms) == (time_t)-1)
      throw InvalidDateFormat();
    // RFC 3339 date format, e.g. "2007-10-27 13:19:59+02:00"
    if (strftime(buffer, sizeof buffer, "%F %T", &tms) != 19)
      throw InvalidDateFormat();
    std::string tzstring;
    if (tzs)
    {
      std::ostringstream stream;
      stream 
        << tzs
        << std::setw(2) << std::setfill('0') << tzh
        << ":"
        << std::setw(2) << std::setfill('0') << tzm
      ;
      tzstring = stream.str();
      if (tzstring.length() != 6)
        throw InvalidDateFormat();
    } 
    stream << *pkey << "\t\"" << buffer << tzstring << "\"" << std::endl;
  }
  catch (InvalidDateFormat &ex)
  {
    debug(1) << "[Warning] metadata[" << *pkey << "] is not a valid date" << std::endl;
  }
}

class PageFiles
{
protected:
  std::vector<File*> data;
  int n_digits;

  PageFiles(int n) : data(n), n_digits(0)
  { 
    while (n > 0)
    {
      this->n_digits++;
      n /= 10;
    }
    if (this->n_digits < 4)
      this->n_digits = 4;
  }

  virtual std::string get_file_name(int n) const
  {
    std::ostringstream stream;
    stream 
      << "p" 
      << std::setfill('0') << std::setw(this->n_digits) << n
      << ".djvu";
    return stream.str();
  }

  void clean_files()
  {
    for (std::vector<File*>::iterator it = this->data.begin(); it != this->data.end(); it++)
    {
      if (*it != NULL)
      {
        delete *it;
        *it = NULL;
      }
    }
  }

public:
  virtual File &operator[](int n) = 0;

  virtual ~PageFiles()
  {
    clean_files();
  }
};

class TemporaryPageFiles : public PageFiles
{
private:
  TemporaryPageFiles(const TemporaryPageFiles&); // not defined
  TemporaryPageFiles& operator=(const TemporaryPageFiles&); // not defined
protected:
  const TemporaryDirectory *directory;
public:

  explicit TemporaryPageFiles(int n) : PageFiles(n)
  { 
    this->directory = new TemporaryDirectory();
  }

  virtual TemporaryFile &operator[](int n)
  {
    std::vector<File*>::reference tmpfile_ptr = this->data.at(n - 1);
    if (tmpfile_ptr == NULL)
    {
      tmpfile_ptr = new TemporaryFile(*this->directory, this->get_file_name(n));
      tmpfile_ptr->close();
    }
    return *dynamic_cast<TemporaryFile*>(tmpfile_ptr);
  }

  virtual ~TemporaryPageFiles()
  {
    this->clean_files();
    delete this->directory;
  }
};

class IndirectPageFiles : public PageFiles
{
private:
  IndirectPageFiles(const IndirectPageFiles&); // not defined
  IndirectPageFiles& operator=(const IndirectPageFiles&); // not defined
  const Directory &directory;
public:
  IndirectPageFiles(int n, const Directory &directory) : PageFiles(n), directory(directory) {}

  virtual File &operator[](int n)
  {
    std::vector<File*>::reference tmpfile_ptr = this->data.at(n - 1);
    if (tmpfile_ptr == NULL)
    {
      tmpfile_ptr = new File(this->directory, this->get_file_name(n));
      tmpfile_ptr->close();
    }
    return *tmpfile_ptr;
  }
};

class DjVm
{
public:
  virtual void add(const File &file) = 0;
  virtual void create() = 0;
  DjVm &operator <<(const File &file)
  {
    this->add(file);
    return *this;
  }
  virtual void set_outline(File &outline_sed_file) = 0;
  virtual ~DjVm() { /* just to shut up compilers */ }
};

class BundledDjVm : public DjVm
{
private:
  File &output_file;
  Command command;
public:
  explicit BundledDjVm(File &output_file) 
  : output_file(output_file),
    command(DJVULIBRE_BIN_PATH "/djvm")
  {
    this->command << "-c" << this->output_file;
  }

  virtual void add(const File &file)
  {
    this->command << file;
  }

  virtual void set_outline(File &outlines_sed_file)
  {
    Command djvused(DJVULIBRE_BIN_PATH "/djvused");
    djvused << "-s" << "-f" << outlines_sed_file << output_file;
    djvused(); // djvused -s -f <outlines-sed-file> <output-djvu-file>
  }

  virtual void create()
  {
    this->command();
  }
};

static const char DJVU_BINARY_TEMPLATE[] = "AT&TFORM\0\0\0\0DJVMDIRM\0\0\0";
static const unsigned char DJVU_VERSION = 1;

static const char DJVU_DUMMY_SINGLE_HEAD[12] = 
{
  0x41, 0x54, 0x26, 0x54, 0x46, 0x4f, 0x52, 0x4d,
  0x00, 0x00, 0x00, 0x20
};

static const char DJVU_DUMMY_DOUBLE_HEAD[48] = 
{
  0x41, 0x54, 0x26, 0x54, 0x46, 0x4f, 0x52, 0x4d, 
  0x00, 0x00, 0x00, 0x74, 0x44, 0x4a, 0x56, 0x4d,
  0x44, 0x49, 0x52, 0x4d, 0x00, 0x00, 0x00, 0x18,
  0x81, 0x00, 0x02, 0x00, 0x00, 0x00, 0x30, 0x00,
  0x00, 0x00, 0x58, 0xff, 0xff, 0xf2, 0xbf, 0x34,
  0x7b, 0xf3, 0x10, 0x74, 0x07, 0x45, 0xc5, 0x40
};

static const char DJVU_DUMMY_DATA[32] =
{ 0x44, 0x4a, 0x56, 0x55, 0x49, 0x4e, 0x46, 0x4f,
  0x00, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x01,
  0x18, 0x00, 0x2c, 0x01, 0x16, 0x00, 0x53, 0x6a,
  0x62, 0x7a, 0x00, 0x00, 0x00, 0x02, 0xbb, 0x7f
};

class IndirectDjVm : public DjVm
{
private:
  File &index_file;
  File *outline_sed_file;
  std::vector<std::string> components;
public:
  explicit IndirectDjVm(File &index_file) 
  : index_file(index_file),
    outline_sed_file(NULL)
  {}

  virtual void add(const File &file)
  {
    std::string name = file;
    size_t pos = name.rfind('/');
    if (pos != std::string::npos)
      name.replace(0, pos + 1, "");
    this->components.push_back(name);
  }

  virtual void set_outline(File &outlines_sed_file)
  {
    TemporaryFile dummy_djvu_file;
    dummy_djvu_file.write(DJVU_DUMMY_DOUBLE_HEAD, sizeof DJVU_DUMMY_DOUBLE_HEAD);
    for (int i = 0; i < 2; i++)
    {
      dummy_djvu_file.write("FORM", 4);
      for (int i = 3; i >= 0; i--)
        dummy_djvu_file << static_cast<char>(((sizeof DJVU_DUMMY_DATA) >> (8 * i)) & 0xff);
      dummy_djvu_file.write(DJVU_DUMMY_DATA, sizeof DJVU_DUMMY_DATA);
    }
    dummy_djvu_file.close();
    Command djvused(DJVULIBRE_BIN_PATH "/djvused");
    djvused << "-s" << "-f" << outlines_sed_file << dummy_djvu_file;
    djvused(); // djvused -s -f <outlines-sed-file> <dummy-djvu-file>
    dummy_djvu_file.reopen();
    dummy_djvu_file.seekg(0x30, std::ios::beg);
    {
      char buffer[4];
      dummy_djvu_file.read(buffer, 4);
      if (std::string(buffer, 4) == std::string("NAVM"))
      {
        size_t navm_size = 0, size = 0;
        for (int i = 3; i >= 0; i--)
        { 
          char c;
          dummy_djvu_file.read(&c, 1);
          navm_size |= static_cast<size_t>(c) << (8 * i);
        }
        navm_size += 8;
        dummy_djvu_file.seekg(0x30, std::ios::beg);
        index_file.reopen();
        this->index_file.seekg(0, std::ios::end);
        copy_stream(dummy_djvu_file, this->index_file, false, navm_size);
        this->index_file.seekg(8, std::ios::beg);
        for (int i = 3; i >= 0; i--)
        { 
          char c;
          this->index_file.read(&c, 1);
          size |= static_cast<size_t>(c) << (8 * i);
        }
        size += navm_size;
        this->index_file.seekg(8, std::ios::beg);
        for (int i = 3; i >= 0; i--)
          this->index_file << static_cast<char>((size >> (8 * i)) & 0xff);
      }
    }
  }

  virtual void create()
  {
    size_t size = this->components.size();
    index_file.write(DJVU_BINARY_TEMPLATE, sizeof DJVU_BINARY_TEMPLATE);
    index_file << DJVU_VERSION;
    for (int i = 1; i >= 0; i--)
      index_file << static_cast<char>((size >> (8 * i)) & 0xff);
    {
      TemporaryFile bzz_file;
      for (size_t i = 0; i < size; i++)
        bzz_file.write("\0\0", 3);
      for (size_t i = 0; i < size; i++)
        bzz_file << '\1';
      for (std::vector<std::string>::const_iterator it = this->components.begin(); it != this->components.end(); it++)
        bzz_file << *it << '\0';
      bzz_file.close();
      Command bzz(DJVULIBRE_BIN_PATH "/bzz");
      bzz << "-e" << bzz_file << "-";
      bzz(index_file);
    }
    index_file.seekg(0, std::ios::end);
    size = index_file.tellg();
    index_file.seekg(8, std::ios::beg);
    for (int i = 3; i >= 0; i--)
      index_file << static_cast<char>(((size - 12) >> (8 * i)) & 0xff);
    index_file.seekg(20, std::ios::beg);
    for (int i = 3; i >= 0; i--)
      index_file << static_cast<char>(((size - 24) >> (8 * i)) & 0xff);
    index_file.close();
  }
};

static int xmain(int argc, char * const argv[])
{
  std::ios_base::sync_with_stdio(false);

  try
  {
    read_config(argc, argv);
  }
  catch (const ConfigurationError &ex)
  {
    usage(ex);
  }

  init_global_params();
  if (!set_antialias(conf_antialias))
    throw Error();

  PDFDoc *doc = new_document(file_name);
  if (!doc->isOk())
    throw Error("Unable to load document");

  debug(1) << doc->getFileName()->getCString() << ":" << std::endl;

  SplashColor paper_color;
  set_color(paper_color, 0xff, 0xff, 0xff);

  int n_pages = doc->getNumPages();
  int page_counter = 0;
  std::auto_ptr<const Directory> output_dir;
  std::auto_ptr<File> output_file;
  std::auto_ptr<DjVm> djvm;
  std::auto_ptr<PageFiles> page_files;
  if (conf_format == CONF_FORMAT_BUNDLED)
  {
    if (conf_output_stdout)
      output_file.reset(new TemporaryFile());
    else
      output_file.reset(new File(conf_output));
    djvm.reset(new BundledDjVm(*output_file));
    page_files.reset(new TemporaryPageFiles(n_pages));
  }
  else
  {
    output_dir.reset(new Directory(conf_output));
    output_file.reset(new File(*output_dir, "index.djvu"));
    page_files.reset(new IndirectPageFiles(n_pages, *output_dir));
    djvm.reset(new IndirectDjVm(*output_file));
  }
  if (conf_pages.size() == 0)
    conf_pages.push_back(std::make_pair(1, n_pages));
  std::map<int, int> page_map;
  int opage = 1;
  for (std::vector< std::pair<int, int> >::iterator page_range = conf_pages.begin(); page_range != conf_pages.end(); page_range++)
  for (int ipage = page_range->first; ipage <= n_pages && ipage <= page_range->second; ipage++)
  {
    page_map[ipage] = opage;
    opage++;
  }
  Renderer *out1 = new Renderer(paper_color);
  MutedRenderer *outm = new MutedRenderer(paper_color, page_map);
  MutedRenderer *outs = new MutedRenderer(paper_color, page_map);
  out1->startDoc(doc->getXRef());
  outm->startDoc(doc->getXRef());
  outs->startDoc(doc->getXRef());
  for (std::vector< std::pair<int, int> >::iterator page_range = conf_pages.begin(); page_range != conf_pages.end(); page_range++)
  for (int n = page_range->first; n <= n_pages && n <= page_range->second; n++)
  {
    page_counter++;
    File &page_file = (*page_files)[n];
    debug(1) << "- page #" << n << " -> #" << page_map[n];
    debug(2) << ":";
    debug(1) << std::endl;
    debug(2) << "  - muted render" << std::endl;
    display_page(doc, outm, n, conf_dpi, conf_dpi, true);
    int width = outm->getBitmapWidth();
    int height = outm->getBitmapHeight();
    debug(2) << "  - image size: " << width << "x" << height << std::endl;
    if (!conf_no_render)
    {
      debug(2) << "  - verbose render" << std::endl;
      display_page(doc, out1, n, conf_dpi, conf_dpi, false);
    }
    debug(2) << "  - create sep_file" << std::endl;
    TemporaryFile sep_file;
    sep_file << "R6 " << width << " " << height << " 216" << std::endl;
    debug(2) << "  - rle palette >> sep_file" << std::endl;
    for (int r = 0; r < 6; r++)
    for (int g = 0; g < 6; g++)
    for (int b = 0; b < 6; b++)
    {
      char buffer[] = { 51 * r, 51 * g, 51 * b };
      sep_file.write(buffer, 3);
    }
    bool has_background = false;
    int background_color[3];
    bool has_foreground = false;
    bool has_text = false;
    if (conf_no_render)
    {
      debug(2) << "  - dummy rle data >> sep_file" << std::endl;
      int item = (0xfff << 20) + width;
      for (int y = 0; y < height; y++)
      for (int i = 0; i < 4; i++)
      {
        char c = item >> ((3 - i) * 8);
        sep_file.write(&c, 1);
      }
    }
    else
    {
      debug(2) << "  - rle data >> sep_file" << std::endl;
      Pixmap bmp1 = Pixmap(out1);
      Pixmap bmpm = Pixmap(outm);
      PixmapIterator p1 = bmp1.begin();
      PixmapIterator pm = bmpm.begin();
      for (int i = 0; i < 3; i++) 
        background_color[i] = pm[i];
      for (int y = 0; y < height; y++)
      {
        int new_color, color = 0xfff;
        int length = 0;
        for (int x = 0; x < width; x++)
        {
          if (!has_background)
          {
            for (int i = 0; i < 3; i++)
            if (background_color[i] != pm[i])
            {
              has_background = true;
              break;
            }
          }
          if (p1[0] != pm[0] || p1[1] != pm[1] || p1[2] != pm[2])
          {
            if (!has_foreground && (p1[0] || p1[1] || p1[2]))
              has_foreground = true;
            new_color = (p1[2] / 51) + 6 * ((p1[1] / 51) + 6 * (p1[0] / 51));
          }
          else
            new_color = 0xfff;
          if (color == new_color)
            length++;
          else
          {
            if (length > 0)
            {
              int item = (color << 20) + length;
              for (int i = 0; i < 4; i++)
              {
                char c = item >> ((3 - i) * 8);
                sep_file.write(&c, 1);
              }
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
    }
    bool nonwhite_background_color;
    if (has_background)
    {
      int sub_width = (width + conf_bg_subsample - 1) / conf_bg_subsample;
      int sub_height = (height + conf_bg_subsample - 1) / conf_bg_subsample;
      double hdpi = sub_width / doc->getPageMediaWidth(n) * 72.0;
      double vdpi = sub_height / doc->getPageMediaHeight(n) * 72.0;
      debug(2) << "  - subsampled render" << std::endl;
      display_page(doc, outs, n, hdpi, vdpi, true);
      if (sub_width != outs->getBitmapWidth())
        throw Error();
      if (sub_height != outs->getBitmapHeight())
        throw Error();
      Pixmap bmp = Pixmap(outs);
      debug(2) << "  - background pixmap >> sep_file" << std::endl;
      sep_file << "P6 " << sub_width << " " << sub_height << " 255" << std::endl;
      sep_file << bmp;
      nonwhite_background_color = false;
      outs->clear_texts();
      outs->clear_annotations();
    }
    else  
    {
      nonwhite_background_color = (background_color[0] & background_color[1] & background_color[2] & 0xff) != 0xff;
      if (nonwhite_background_color)
      {
        // Dummy background just to assure FGbz chunks.
        // It will be replaced later.
        int sub_width = (width + 10) / 11;
        int sub_height = (height + 10) / 11;
        debug(2) << "  - dummy background pixmap >> sep_file" << std::endl;
        sep_file << "P6 " << sub_width << " " << sub_height << " 255" << std::endl;
        for (int x = 0; x < sub_width; x++)
        for (int y = 0; y < sub_height; y++)
          sep_file.write("\xff\xff\xff", 3);
      }
    }
    if (conf_text)
    {
      debug(2) << "  - text layer >> sep_file" << std::endl;
      const std::vector<std::string> &texts = outm->get_texts();
      for (std::vector<std::string>::const_iterator it = texts.begin(); it != texts.end(); it++)
      {
        if (it->size() == 0)
          continue;
        sep_file << *it;
        has_text = true;
      }
      outm->clear_texts();
    }
    sep_file.close();
    {
      debug(2) << "  - !csepdjvu" << std::endl;
      Command csepdjvu(DJVULIBRE_BIN_PATH "/csepdjvu");
      csepdjvu << "-d" << conf_dpi;
      if (conf_bg_slices)
        csepdjvu << "-q" << conf_bg_slices;
      if (conf_text == CONF_TEXT_LINES)
        csepdjvu << "-t";
      csepdjvu << sep_file << page_file;
      csepdjvu();
    }
    *djvm << page_file;
    TemporaryFile sjbz_file, fgbz_file, bg44_file, sed_file;
    { 
      debug(2) << "  - !djvuextract" << std::endl;
      Command djvuextract(DJVULIBRE_BIN_PATH "/djvuextract");
      djvuextract << page_file;
      if (has_background || has_foreground || nonwhite_background_color)
        djvuextract
          << std::string("FGbz=") + std::string(fgbz_file)
          << std::string("BG44=") + std::string(bg44_file);
      djvuextract << std::string("Sjbz=") + std::string(sjbz_file);
      djvuextract(conf_verbose < 2);
    }
    if (nonwhite_background_color)
    {
      TemporaryFile c44_file;
      {
        TemporaryFile ppm_file;
        debug(2) << "  - !c44" << std::endl;
        Command c44(DJVULIBRE_BIN_PATH "/c44");
        c44 << "-slice" << "97" << ppm_file << c44_file;
        int bg_width = (width + 11) / 12;
        int bg_height = (height + 11) / 12;
        ppm_file << "P6 " << bg_width << " " << bg_height << " 255" << std::endl;
        for (int y = 0; y < bg_height; y++)
        for (int x = 0; x < bg_width; x++)
        for (int i = 0; i < 3; i++)
        {
          char c = background_color[i];
          ppm_file.write(&c, 1);
        }
        ppm_file.close();
        c44();
      }
      {
        c44_file.reopen();
        debug(2) << "  - !djvuextract" << std::endl;
        Command djvuextract(DJVULIBRE_BIN_PATH "/djvuextract");
        djvuextract << c44_file << std::string("BG44=") + std::string(bg44_file);
        djvuextract(conf_verbose < 2);
      }
    }
    {
      debug(2) << "  - annotations >> sed_file" << std::endl;
      const std::vector<std::string> &annotations = outm->get_annotations();
      sed_file << "select 1" << std::endl << "set-ant" << std::endl;
      for (std::vector<std::string>::const_iterator it = annotations.begin(); it != annotations.end(); it++)
        sed_file << *it << std::endl;
      sed_file << "." << std::endl;
      outm->clear_annotations();
    }
    if (has_text)
    {
      debug(2) << "  - !djvused >> sed_file" << std::endl;
      Command djvused(DJVULIBRE_BIN_PATH "/djvused");
      djvused << page_file << "-e" << "output-txt";
      djvused(sed_file);
    }
    sed_file.close();
    {
      debug(2) << "  - !djvumake" << std::endl;
      Command djvumake(DJVULIBRE_BIN_PATH "/djvumake");
      std::ostringstream info;
      info << "INFO=" << width << "," << height << "," << conf_dpi;
      djvumake
        << page_file
        << info.str()
        << std::string("Sjbz=") + std::string(sjbz_file);
      if (has_foreground || has_background || nonwhite_background_color)
        djvumake
          << std::string("FGbz=") + std::string(fgbz_file)
          << std::string("BG44=") + std::string(bg44_file);
      djvumake();
    }
    {
      debug(2) << "  - !djvused < sed_file" << std::endl;
      Command djvused(DJVULIBRE_BIN_PATH "/djvused");
      djvused << page_file << "-s" << "-f" << sed_file;
      djvused();
    }
  }
  if (page_counter == 0)
    throw Error("No pages selected");
  {
    TemporaryFile dummy_page_file;
    TemporaryFile sed_file;
    if (page_counter == 1 && conf_format == CONF_FORMAT_BUNDLED)
    {
      // Dummy page is necessary to force multi-file document structure.
      dummy_page_file.write(DJVU_DUMMY_SINGLE_HEAD, sizeof DJVU_DUMMY_SINGLE_HEAD);
      dummy_page_file.write(DJVU_DUMMY_DATA, sizeof DJVU_DUMMY_DATA);
      dummy_page_file.close();
      *djvm << dummy_page_file;
    }
    debug(2) << "- !djvm" << std::endl;
    djvm->create();
  }
  {
    TemporaryFile sed_file;
    if (conf_extract_metadata)
    {
      debug(2) << "- metadata >> sed_file" << std::endl;
      sed_file << "set-meta" << std::endl;
      pdf_metadata_to_djvu_metadata(doc, sed_file);
      sed_file << "." << std::endl;
    }
    sed_file.close();
    debug(2) << "- !djvused < sed_file" << std::endl;
    Command djvused(DJVULIBRE_BIN_PATH "/djvused");
    djvused << *output_file << "-s" << "-f" << sed_file;
    djvused();
  }
  if (conf_extract_outline)
  {
    TemporaryFile sed_file;
    debug(2) << "- outlines >> sed_file" << std::endl;
    if (conf_format == CONF_FORMAT_BUNDLED)
    {
      // Shared annotations chunk in necessary to preserve multi-file document structure.
      // (Single-file documents cannot contain document outline.)
      sed_file << "create-shared-ant" << std::endl;
    }
    sed_file << "set-outline" << std::endl;
    pdf_outline_to_djvu_outline(doc, sed_file, page_map);
    sed_file << std::endl << "." << std::endl;
    sed_file.close();
    djvm->set_outline(sed_file);
  }
  if (page_counter == 1 && conf_format == CONF_FORMAT_BUNDLED)
  {
    // Dummy page is redundant now, so remove it.
    Command djvm(DJVULIBRE_BIN_PATH "/djvm");
    djvm << "-d" << *output_file << "2";
    debug(2) << "- !djvm -d" << std::endl;
    djvm();
  }
  if (conf_output_stdout)
    copy_stream(*output_file, std::cout, true);
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
  catch (std::ios_base::failure &ex)
  {
    std::cerr << "I/O error (" << ex.what() << ")" << std::endl;
    exit(2);
  }
}

// vim:ts=2 sw=2 et
