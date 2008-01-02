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

#include <fcntl.h>
#include <sys/stat.h>
#include <iconv.h>

#include "compoppler.h"
#include "debug.h"
#include "config.h"

#include <pstreams/pstream.h>
#include <libdjvu/miniexp.h>

class OSError : public Error
{
public:
  OSError() : Error("")
  {
    message += strerror(errno);
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
    if (!config::extract_hyperlinks)
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
    config::read_config(argc, argv);
  }
  catch (const config::Error &ex)
  {
    config::usage(ex);
    exit(1);
  }

  init_global_params();
  if (!set_antialias(config::antialias))
    throw Error();

  PDFDoc *doc = new_document(config::file_name);
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
  if (config::format == config::FORMAT_BUNDLED)
  {
    if (config::output_stdout)
      output_file.reset(new TemporaryFile());
    else
      output_file.reset(new File(config::output));
    djvm.reset(new BundledDjVm(*output_file));
    page_files.reset(new TemporaryPageFiles(n_pages));
  }
  else
  {
    output_dir.reset(new Directory(config::output));
    output_file.reset(new File(*output_dir, "index.djvu"));
    page_files.reset(new IndirectPageFiles(n_pages, *output_dir));
    djvm.reset(new IndirectDjVm(*output_file));
  }
  if (config::pages.size() == 0)
    config::pages.push_back(std::make_pair(1, n_pages));
  std::map<int, int> page_map;
  int opage = 1;
  for (std::vector< std::pair<int, int> >::iterator page_range = config::pages.begin(); page_range != config::pages.end(); page_range++)
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
  for (std::vector< std::pair<int, int> >::iterator page_range = config::pages.begin(); page_range != config::pages.end(); page_range++)
  for (int n = page_range->first; n <= n_pages && n <= page_range->second; n++)
  {
    page_counter++;
    File &page_file = (*page_files)[n];
    debug(1) << "- page #" << n << " -> #" << page_map[n];
    debug(2) << ":";
    debug(1) << std::endl;
    debug(2) << "  - muted render" << std::endl;
    display_page(doc, outm, n, config::dpi, config::dpi, true);
    int width = outm->getBitmapWidth();
    int height = outm->getBitmapHeight();
    debug(2) << "  - image size: " << width << "x" << height << std::endl;
    if (!config::no_render)
    {
      debug(2) << "  - verbose render" << std::endl;
      display_page(doc, out1, n, config::dpi, config::dpi, false);
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
    if (config::no_render)
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
            new_color = ((p1[2] + 1) / 43) + 6 * (((p1[1] + 1) / 43) + 6 * ((p1[0] + 1) / 43));
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
      int sub_width = (width + config::bg_subsample - 1) / config::bg_subsample;
      int sub_height = (height + config::bg_subsample - 1) / config::bg_subsample;
      double hdpi = sub_width / get_page_width(doc, n);
      double vdpi = sub_height / get_page_height(doc, n);
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
    if (config::text)
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
      csepdjvu << "-d" << config::dpi;
      if (config::bg_slices)
        csepdjvu << "-q" << config::bg_slices;
      if (config::text == config::TEXT_LINES)
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
      djvuextract(config::verbose < 2);
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
        djvuextract(config::verbose < 2);
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
      info << "INFO=" << width << "," << height << "," << config::dpi;
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
    if (page_counter == 1 && config::format == config::FORMAT_BUNDLED)
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
    if (config::extract_metadata)
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
  if (config::extract_outline)
  {
    TemporaryFile sed_file;
    debug(2) << "- outlines >> sed_file" << std::endl;
    if (config::format == config::FORMAT_BUNDLED)
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
  if (page_counter == 1 && config::format == config::FORMAT_BUNDLED)
  {
    // Dummy page is redundant now, so remove it.
    Command djvm(DJVULIBRE_BIN_PATH "/djvm");
    djvm << "-d" << *output_file << "2";
    debug(2) << "- !djvm -d" << std::endl;
    djvm();
  }
  if (config::output_stdout)
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
