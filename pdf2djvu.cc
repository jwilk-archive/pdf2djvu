/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include "compoppler.hh"
#include "config.hh"
#include "debug.hh"
#include "djvuconst.hh"
#include "quantizer.hh"
#include "sexpr.hh"
#include "system.hh"
#include "version.hh"

Config config;

static inline DebugStream &debug(int n)
{
  return debug(n, config.verbose);
}

class NoLinkDestination : public std::runtime_error
{
public:
  NoLinkDestination() : std::runtime_error("Cannot find link destination") {}
};

static int get_page_for_LinkGoTo(pdf::link::GoTo *goto_link, pdf::Catalog *catalog)
{
  std::auto_ptr<pdf::link::Destination> dest;
  dest.reset(goto_link->getDest());
  if (dest.get() == NULL)
    dest.reset(catalog->findDest(goto_link->getNamedDest()));
  else
    dest.reset(dest.release()->copy());
  if (dest.get() != NULL)
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

static bool is_foreground_color_map(pdf::gfx::ImageColorMap *color_map)
{
  return (color_map->getNumPixelComps() <= 1 && color_map->getBits() <= 1);
}

class PageFiles
{
protected:
  std::vector<File*> data;
  int n_digits;
  std::string prefix;

  PageFiles(int n, const std::string &prefix) : data(n), n_digits(0), prefix(prefix)
  { 
    while (n > 0)
    {
      this->n_digits++;
      n /= 10;
    }
    if (this->n_digits < 4)
      this->n_digits = 4;
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

  std::string get_file_name(int n) const
  {
    if (n <= 0)
      return "";
    std::ostringstream stream;
    stream 
      << prefix
      << std::setfill('0') << std::setw(this->n_digits) << n
      << ".djvu";
    return stream.str();
  }

  virtual ~PageFiles() throw ()
  {
    this->clean_files();
  }
};

class MutedRenderer: public pdf::Renderer
{
private:
  std::auto_ptr<std::ostringstream> text_comments;
  std::vector<sexpr::Ref> annotations;
  const PageFiles &page_files;
protected:

  void add_text_comment(int ox, int oy, int dx, int dy, int x, int y, int w, int h, const Unicode *unistr, int len)
  {
    while (len > 0 && *unistr == ' ')
      unistr++, len--;
    if (len == 0)
      return;
    *(this->text_comments)
      << std::dec << std::noshowpos
      << "# T " 
      << ox << ":" << oy << " " 
      << dx << ":" << dy << " "
      <<  w << "x" <<  h << std::showpos << x << y << " "
      << "("
      << std::oct;
    for (; len > 0; len--, unistr++)
    {
      if (*unistr < 0x20 || *unistr == ')' || *unistr == '\\')
        *(this->text_comments) << "\\" << std::setw(3) << static_cast<unsigned int>(*unistr);
      else
        pdf::write_as_utf8(*(this->text_comments), *unistr);
    }
    *(this->text_comments) << ")" << std::endl;
  }
public:

  void drawImageMask(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height, 
    GBool invert, GBool inline_image)
  {
    return;
  }

  void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, int *mask_colors, GBool inline_image)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
      return;
    Renderer::drawImage(state, object, stream, width, height, color_map, mask_colors, inline_image);
  }

  void drawMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Stream *mask_stream, int mask_width, int mask_height, GBool mask_invert)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
      return;
    Renderer::drawMaskedImage(state, object, stream, width, height,
      color_map, mask_stream, mask_width, mask_height, mask_invert);
  }

  void drawSoftMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream,
    int width, int height, pdf::gfx::ImageColorMap *color_map, pdf::Stream *mask_stream,
    int mask_width, int mask_height,	pdf::gfx::ImageColorMap *mask_color_map)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
      return;
    Renderer::drawSoftMaskedImage(state, object, stream, width, height,
      color_map, mask_stream, mask_width, mask_height, mask_color_map);
  }

  GBool interpretType3Chars() { return gFalse; }

  void drawChar(pdf::gfx::State *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
    CharCode code, int n_bytes, Unicode *unistr, int length)
  {
    double pox, poy, pdx, pdy, px, py, pw, ph;
    x -= origin_x; y -= origin_y;
    state->transform(x, y, &pox, &poy);
    state->transformDelta(dx, dy, &pdx, &pdy);
    int old_render = state->getRender();
    state->setRender(0x103);
    this->Renderer::drawChar(state, x, y, dx, dy, origin_x, origin_y, code, n_bytes, unistr, length);
    state->setRender(old_render);
    pdf::splash::Font *font = this->getCurrentFont();
    pdf::splash::GlyphBitmap glyph;
    px = pox; py = poy;
    if (pdf::get_glyph(this->getSplash(), font, pox, poy, code, &glyph))
    {
      px -= glyph.x;
      py -= glyph.y;
      pw = glyph.w;
      ph = glyph.h;
    }
    else
    {
      // Ideally, this should not happen. Some heuristics is required to
      // determine character width/height.
      pw = pdx; ph = pdy;
      double font_size = state->getTransformedFontSize();
      if (pw * 4.0 < font_size)
        pw = font_size;
      if (ph * 4.0 < font_size)
        ph = font_size;
      py -= ph;
    }
    pw = std::max(pw, 1.0);
    ph = std::max(ph, 1.0);
    std::auto_ptr<pdf::NFKC> nfkc;
    const Unicode *const_unistr;
    if (config.text_nfkc)
    {
      nfkc.reset(new pdf::NFKC(unistr, length));
      const_unistr = *nfkc;
      length = nfkc->length();
    }
    else
      const_unistr = unistr;
    add_text_comment(
      static_cast<int>(pox),
      static_cast<int>(poy),
      static_cast<int>(pdx),
      static_cast<int>(pdy),
      static_cast<int>(px),
      static_cast<int>(py),
      static_cast<int>(pw),
      static_cast<int>(ph),
      const_unistr, length
    );
  }

  void drawLink(pdf::link::Link *link, const std::string &border_color, pdf::Catalog *catalog)
  {
    sexpr::GCLock gc_lock;
    if (!config.extract_hyperlinks)
      return;
    double x1, y1, x2, y2;
    pdf::link::Action *link_action = link->getAction();
    std::string uri;
    link->getRect(&x1, &y1, &x2, &y2);
    switch (link_action->getKind())
    {
    case actionURI:
      uri += dynamic_cast<pdf::link::URI*>(link_action)->getURI()->getCString();
      break;
    case actionGoTo:
    {
      int page;
      try
      {
        page = get_page_for_LinkGoTo(dynamic_cast<pdf::link::GoTo*>(link_action), catalog);
      }
      catch (NoLinkDestination &ex)
      {
        debug(1) << "[Warning] " << ex << std::endl;
        return;
      }
      std::ostringstream strstream;
      strstream << "#" << this->page_files.get_file_name(page);
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
    sexpr::Ref expr = sexpr::nil;
    for (
      std::vector<sexpr::Ref>::const_iterator it = config.hyperlinks_options.begin();
      it != config.hyperlinks_options.end(); it++
    )
      expr = sexpr::cons(*it, expr);
    if (!config.hyperlinks_user_border_color)
    { 
      static sexpr::Ref symbol_xor = sexpr::symbol("xor");
      static sexpr::Ref symbol_border = sexpr::symbol("border");
      sexpr::Ref bexpr = sexpr::nil;
      if (border_color.empty())
        bexpr = sexpr::cons(symbol_xor, bexpr);
      else
      {
        bexpr = sexpr::cons(sexpr::symbol(border_color), bexpr);
        bexpr = sexpr::cons(symbol_border, bexpr);
      }
      expr = sexpr::cons(bexpr, expr);
    };
    static sexpr::Ref symbol_rect = sexpr::symbol("rect");
    static sexpr::Ref symbol_maparea = sexpr::symbol("maparea");
    {
      sexpr::Ref rexpr = sexpr::nil;
      rexpr = sexpr::cons(sexpr::integer(h), rexpr);
      rexpr = sexpr::cons(sexpr::integer(w), rexpr);
      rexpr = sexpr::cons(sexpr::integer(y), rexpr);
      rexpr = sexpr::cons(sexpr::integer(x), rexpr);
      rexpr = sexpr::cons(symbol_rect, rexpr);
      expr = sexpr::cons(rexpr, expr);
    }
    expr = sexpr::cons(sexpr::empty_string, expr);
    expr = sexpr::cons(sexpr::string(uri), expr);
    expr = sexpr::cons(symbol_maparea, expr);
    annotations.push_back(expr);
  }

  GBool useDrawChar() { return gTrue; }

  void stroke(pdf::gfx::State *state) { }
  void fill(pdf::gfx::State *state)
  { 
    if (config.no_render)
      return;
    pdf::splash::Path path;
    this->convert_path(state, path);
    double area = pdf::get_path_area(path);
    if (area / this->getBitmapHeight() / this->getBitmapWidth() >= 0.8)
      Renderer::fill(state);
  }
  void eoFill(pdf::gfx::State *state) { }

  MutedRenderer(pdf::splash::Color &paper_color, bool monochrome, const PageFiles &page_files) 
  : Renderer(paper_color, monochrome), page_files(page_files)
  { 
    this->clear_texts();
  }

  const std::vector<sexpr::Ref> &get_annotations() const
  {
    return annotations;
  }

  void clear_annotations()
  {
    annotations.clear();
  }

  const std::string get_texts() const
  {
    return this->text_comments->str();
  }

  void clear_texts()
  {
    this->text_comments.reset(new std::ostringstream);
    *(this->text_comments) << std::setfill('0');
  }
};

class BookmarkError : public std::runtime_error
{
public:
  BookmarkError(const std::string &message) : std::runtime_error(message) {}
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

namespace pdf
{
  // [TODO] move the following functions to `compoppler`

  static std::string string_as_utf8(pdf::String *string)
  {
    const char *cstring = string->getCString();
    std::ostringstream stream;
    if ((cstring[0] & 0xff) == 0xfe && (cstring[1] & 0xff) == 0xff)
      utf16_to_utf8(cstring, string->getLength(), stream);
    else
      pdf::write_as_utf8(stream, cstring);
    return stream.str();
  }

  static inline std::string string_as_utf8(pdf::Object &object)
  {
    return pdf::string_as_utf8(object.getString());
  }

}

static sexpr::Expr pdf_outline_to_djvu_outline(pdf::Object *node, pdf::Catalog *catalog,
  const PageFiles &page_files)
{
  sexpr::GCLock gc_lock;
  pdf::Object current, next;
  if (!pdf::dict_lookup(node, "First", &current)->isDict())
    return sexpr::nil;
  sexpr::Ref list = sexpr::nil;
  while (current.isDict())
  {
    try
    {
      std::string title_str;
      {
        pdf::OwnedObject title;
        if (!pdf::dict_lookup(current, "Title", &title)->isString())
          throw NoTitleForBookmark();
        title_str = pdf::string_as_utf8(title);
      } 

      int page;
      {
        pdf::OwnedObject destination;
        std::auto_ptr<pdf::link::Action> link_action;
        if (!pdf::dict_lookup(current, "Dest", &destination)->isNull())
          link_action.reset(pdf::link::Action::parseDest(&destination));
        else if (!pdf::dict_lookup(current, "A", &destination)->isNull())
          link_action.reset(pdf::link::Action::parseAction(&destination));
        else
          throw NoPageForBookmark();
        if (link_action.get() == NULL || link_action->getKind() != actionGoTo)
          throw NoPageForBookmark();
        page = get_page_for_LinkGoTo(dynamic_cast<pdf::link::GoTo*>(link_action.get()), catalog);
      }
      sexpr::Ref expr = sexpr::nil;
      expr = pdf_outline_to_djvu_outline(&current, catalog, page_files);
      {
        std::ostringstream strstream;
        strstream << "#" << page_files.get_file_name(page);
        sexpr::Ref pexpr = sexpr::string(strstream.str());
        expr = sexpr::cons(pexpr, expr);
      }
      expr = sexpr::cons(sexpr::string(title_str), expr);
      list = sexpr::cons(expr, list);
    }
    catch (BookmarkError &ex)
    {
      debug(1) << "[Warning] " << ex << std::endl;
    }

    pdf::dict_lookup(current, "Next", &next);
    current.free();
    current = next;
  }
  current.free();
  list.reverse();
  return list;
}

static bool pdf_outline_to_djvu_outline(pdf::Document &doc, std::ostream &stream,
  const PageFiles &page_files)
/* Convert the PDF outline to DjVu outline. Save the resulting S-expression
 * into the stream.
 *
 * Return ``true`` if the outline exist and is non-empty.
 * Return ``false`` otherwise.
 */
{
  sexpr::GCLock gc_lock;
  pdf::Catalog *catalog = doc.getCatalog();
  pdf::Object *outlines = catalog->getOutline();
  if (!outlines->isDict())
    return false;
  static sexpr::Ref symbol_bookmarks = sexpr::symbol("bookmarks");
  sexpr::Ref expr = pdf_outline_to_djvu_outline(outlines, catalog, page_files);
  if (expr == sexpr::nil)
    return false;
  expr = sexpr::cons(symbol_bookmarks, expr);
  stream << expr;
  return true;
}

class InvalidDateFormat : public std::runtime_error
{ 
public:
  InvalidDateFormat()
  : std::runtime_error("Invalid date format")
  { }
};

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

static void pdf_metadata_to_djvu_metadata(pdf::Document &doc, std::ostream &stream)
{
  static const char* string_keys[] = { "Title", "Subject", "Keywords", "Author", "Creator", NULL };
  static const char* date_keys[] = { "CreationDate", "ModDate", NULL };
  pdf::OwnedObject info;
  doc.getDocInfo(&info);
  if (!info.isDict())
    return;
  pdf::Dict *info_dict = info.getDict();
  for (const char** pkey = string_keys; *pkey; pkey++)
  {
    pdf::OwnedObject object;
    if (!pdf::dict_lookup(info_dict, *pkey, &object)->isString())
      continue;
    try
    {
      std::string value = pdf::string_as_utf8(object);
      sexpr::Ref esc_value = sexpr::string(value);
      stream << *pkey << "\t" << esc_value << std::endl;
    }
    catch (IconvError &ex)
    {
      debug(1) << "[Warning] metadata[" << *pkey << "] is not properly encoded" << std::endl;
      continue;
    }
  }
  {
    bool have_producer = false;
    std::string value;
    pdf::OwnedObject object;
    if (pdf::dict_lookup(info_dict, "Producer", &object)->isString())
    {
      have_producer = true;
      try
      {
        value = pdf::string_as_utf8(object);
        if (config.adjust_metadata && value.length() > 0)
          value += "\n" PACKAGE_STRING;
      }
      catch (IconvError &ex)
      {
        debug(1) << "[Warning] metadata[Producer] is not properly encoded" << std::endl;
      }
    }
    else if (config.adjust_metadata)
    {
      have_producer = true;
      value = PACKAGE_STRING;
    }
    if (have_producer)
    {
      sexpr::Ref esc_value = sexpr::string(value);
      stream << "Producer\t" << esc_value << std::endl;
    }
  }
  for (const char** pkey = date_keys; *pkey; pkey++)
  try
  {
    pdf::OwnedObject object;
    struct tm tms;
    char tzs; int tzh = 0, tzm = 0;
    char buffer[32];
    if (!pdf::dict_lookup(info_dict, *pkey, &object)->isString())
      continue;
    char *input = object.getString()->getCString();
    if (input[0] == 'D' && input[1] == ':')
      input += 2;
    tms.tm_isdst = -1;
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
    if (strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S", &tms) != 19)
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


class TemporaryPageFiles : public PageFiles
{
private:
  TemporaryPageFiles(const TemporaryPageFiles&); // not defined
  TemporaryPageFiles& operator=(const TemporaryPageFiles&); // not defined
protected:
  const TemporaryDirectory *directory;
public:

  explicit TemporaryPageFiles(int n, const std::string &prefix) : PageFiles(n, prefix)
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

  virtual ~TemporaryPageFiles() throw ()
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
  IndirectPageFiles(int n, const Directory &directory, const std::string &prefix) : PageFiles(n, prefix), directory(directory) {}

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

class DjVuCommand : public Command
{
protected:
  static std::string dir_name;
  static std::string full_path(const std::string &base_name)
  {
    return DjVuCommand::dir_name + "/" + base_name;
  } 
public:
  static void set_argv0(const char *argv0);
  explicit DjVuCommand(const std::string &base_name)
  : Command(full_path(base_name))
  { }
};

std::string DjVuCommand::dir_name("");

void DjVuCommand::set_argv0(const char *argv0)
{
  std::string argv0_dir_name, argv0_file_name;
  split_path(argv0, argv0_dir_name, argv0_file_name);
  DjVuCommand::dir_name = absolute_path(DJVULIBRE_BIN_PATH, argv0_dir_name);
}

class DjVm
{
public:
  virtual void add(const File &file) = 0;
  virtual void create() = 0;
  virtual void commit() = 0;
  DjVm &operator <<(const File &file)
  {
    this->add(file);
    return *this;
  }
  virtual void set_outline(File &outline_sed_file) = 0;
  virtual void set_metadata(File &metadata_sed_file) = 0;
  virtual ~DjVm() throw () { /* just to shut up compilers */ }
};

class BundledDjVm : public DjVm
{
private:
  File &output_file;
  DjVuCommand command;
  unsigned int page_counter;
public:
  explicit BundledDjVm(File &output_file) 
  : output_file(output_file),
    command("djvm"),
    page_counter(0)
  {
    this->command << "-c" << this->output_file;
  }

  virtual void add(const File &file)
  {
    this->page_counter++;
    this->command << file;
  }

  virtual void set_outline(File &outlines_sed_file)
  {
    debug(3) << "creating document outline with `djvused`" << std::endl;
    DjVuCommand djvused("djvused");
    djvused << "-s" << "-f" << outlines_sed_file << this->output_file;
    djvused(); // djvused -s -f <outlines-sed-file> <output-djvu-file>
  }

  virtual void set_metadata(File &metadata_sed_file)
  {
    debug(3) << "setting metadata with `djvused`" << std::endl;
    DjVuCommand djvused("djvused");
    djvused << "-s" << "-f" << metadata_sed_file << this->output_file;
    djvused(); // djvused -s -f <metadata-sed-file> <output-djvu-file>
  }

  virtual void create()
  {
    std::auto_ptr<TemporaryFile> dummy_page_file;
    if (this->page_counter == 1)
    {
      /* An additional, dummy page is necessary to force multi-file document structure. */
      dummy_page_file.reset(new TemporaryFile());
      dummy_page_file->write(djvu::DUMMY_SINGLE_HEAD, sizeof djvu::DUMMY_SINGLE_HEAD);
      dummy_page_file->write(djvu::DUMMY_DATA, sizeof djvu::DUMMY_DATA);
      dummy_page_file->close();
      this->command << *dummy_page_file;
    }
    debug(3) << "creating multi-page document (" << this->page_counter << " ";
    if (this->page_counter == 1)
      debug(3) << "page + a dummy page";
    else
      debug(3) << "pages";
    debug(3) << ") with `djvm`" << std::endl;
    this->command();
  }

  virtual void commit()
  {
    if (this->page_counter == 1)
    {
      /* The dummy page is redundant now, so remove it. */
      DjVuCommand djvm("djvm");
      djvm << "-d" << this->output_file << "2";
      debug(3) << "removing the dummy page with `djvm`" << std::endl;
      djvm();
    }
  }
};

class IndirectDjVm : public DjVm
{
private:
  File &index_file;
  std::vector<std::string> components;
  class UnexpectedDjvuSedOutput : public std::runtime_error
  { 
  public: 
    UnexpectedDjvuSedOutput() 
    : std::runtime_error("Unexpected output from djvused") 
    { };
  };
  void do_create(const std::vector<std::string> &components, bool shared_ant = false);
public:
  explicit IndirectDjVm(File &index_file) 
  : index_file(index_file)
  {}

  virtual ~IndirectDjVm() throw ()
  {}

  virtual void add(const File &file)
  {
    this->components.push_back(file.get_basename());
  }

  virtual void set_outline(File &outlines_sed_file)
  {
    TemporaryFile dummy_djvu_file;
    dummy_djvu_file.write(djvu::DUMMY_DOUBLE_HEAD, sizeof djvu::DUMMY_DOUBLE_HEAD);
    for (int i = 0; i < 2; i++)
    {
      dummy_djvu_file.write("FORM", 4);
      for (int i = 3; i >= 0; i--)
        dummy_djvu_file << static_cast<char>(((sizeof djvu::DUMMY_DATA) >> (8 * i)) & 0xff);
      dummy_djvu_file.write(djvu::DUMMY_DATA, sizeof djvu::DUMMY_DATA);
    }
    dummy_djvu_file.close();
    debug(3) << "creating outline with `djvused`" << std::endl;
    DjVuCommand djvused("djvused");
    djvused << "-s" << "-f" << outlines_sed_file << dummy_djvu_file;
    djvused(); // djvused -s -f <outlines-sed-file> <dummy-djvu-file>
    debug(3) << "copying outline" << std::endl;
    dummy_djvu_file.reopen();
    dummy_djvu_file.seekg(0x30, std::ios::beg);
    {
      char buffer[4];
      dummy_djvu_file.read(buffer, 4);
      if (std::string(buffer, 4) == std::string("NAVM"))
      {
        size_t navm_size = 0, chunks_size = 0;
        for (int i = 3; i >= 0; i--)
        { 
          char c;
          dummy_djvu_file.read(&c, 1);
          navm_size |= static_cast<size_t>(c & 0xff) << (8 * i);
        }
        navm_size += 8;
        dummy_djvu_file.seekg(0x30, std::ios::beg);
        index_file.reopen();
        this->index_file.seekg(0, std::ios::end);
        size_t file_size = this->index_file.tellg();
        if (file_size & 1)
          // Each chunk must begin on an even byte boundary
          this->index_file << '\0';
        copy_stream(dummy_djvu_file, this->index_file, false, navm_size);
        this->index_file.seekg(8, std::ios::beg);
        for (int i = 3; i >= 0; i--)
        { 
          char c;
          this->index_file.read(&c, 1);
          chunks_size |= static_cast<size_t>(c & 0xff) << (8 * i);
        }
        chunks_size += navm_size + (file_size & 1);
        this->index_file.seekg(8, std::ios::beg);
        for (int i = 3; i >= 0; i--)
          this->index_file << static_cast<char>((chunks_size >> (8 * i)) & 0xff);
      }
      else
        throw UnexpectedDjvuSedOutput();
    }
  }

  virtual void set_metadata(File &metadata_sed_file)
  {
    debug(3) << "setting metadata with `djvused` (may take much time)" << std::endl;
    DjVuCommand djvused("djvused");
    djvused << "-s" << "-f" << metadata_sed_file << this->index_file;
    djvused(); // djvused -s -f <metadata-sed-file> <output-djvu-file>
  }

  virtual void create()
  {
    size_t size = this->components.size();
    debug(3)
      << "creating multi-page indirect document "
      << "(" << size << " " << (size == 1 ? "page" : "pages") << ")"
      << std::endl;
    this->do_create(this->components);
  }

  virtual void commit()
  { }
};

void IndirectDjVm::do_create(const std::vector<std::string> &components, bool shared_ant)
{
  size_t size = components.size();
  this->index_file.write(djvu::BINARY_TEMPLATE, sizeof djvu::BINARY_TEMPLATE);
  this->index_file << djvu::VERSION;
  for (int i = 1; i >= 0; i--)
    index_file << static_cast<char>(((size + shared_ant) >> (8 * i)) & 0xff);
  {
    TemporaryFile bzz_file;
    for (size_t i = 0; i < size + shared_ant; i++)
      bzz_file.write("\0\0", 3);
    if (shared_ant)
      bzz_file << '\3';
    for (size_t i = 0; i < size; i++)
      bzz_file << '\1';
    if (shared_ant)
      bzz_file << "shared_anno.iff" << '\0';
    for (std::vector<std::string>::const_iterator it = components.begin(); it != components.end(); it++)
      bzz_file << *it << '\0';
    bzz_file.close();
    DjVuCommand bzz("bzz");
    bzz << "-e" << bzz_file << "-";
    bzz(index_file);
  }
  size = this->index_file.size();
  this->index_file.seekg(8, std::ios::beg);
  for (int i = 3; i >= 0; i--)
    this->index_file << static_cast<char>(((size - 12) >> (8 * i)) & 0xff);
  this->index_file.seekg(20, std::ios::beg);
  for (int i = 3; i >= 0; i--)
    this->index_file << static_cast<char>(((size - 24) >> (8 * i)) & 0xff);
  this->index_file.close();
}

static int calculate_dpi(double page_width, double page_height)
{
  if (config.preferred_page_size.first)
  {
    double hdpi = config.preferred_page_size.first / page_width;
    double vdpi = config.preferred_page_size.second / page_height;
    double dpi = std::min(hdpi, vdpi);
    if (dpi < djvu::MIN_DPI)
      return djvu::MIN_DPI;
    else if (dpi > djvu::MAX_DPI)
      return djvu::MAX_DPI;
    else
      return static_cast<int>(dpi);
  }
  else
    return config.dpi;
}

class StdoutIsATerminal : public std::runtime_error
{
public:
  StdoutIsATerminal() 
  : std::runtime_error("I won't write DjVu data to a terminal.")
  { }
};

static void calculate_subsampled_size(int width, int height, int ratio, int &sub_width, int &sub_height)
{
  // See <http://sf.net/tracker/?func=detail&aid=1972089&group_id=32953&atid=406583>
  while (true)
  {
    sub_width = (width + ratio - 1) / ratio;
    sub_height = (height + ratio - 1) / ratio;
    if ((width + sub_width - 1) / sub_width != (height + sub_height - 1) / sub_height)
      ratio--;
    else
      break;
  }
}

static int xmain(int argc, char * const argv[])
{
  std::ios_base::sync_with_stdio(false);
  DjVuCommand::set_argv0(argv[0]);

  try
  {
    config.read_config(argc, argv);
  }
  catch (const Config::NeedVersion)
  {
    error_log << PACKAGE_STRING << std::endl;
    exit(1);
  }
  catch (const Config::Error &ex)
  {
    config.usage(ex);
    exit(1);
  }

  if (config.output_stdout && is_stream_a_tty(std::cout))
    throw StdoutIsATerminal();

  pdf::Environment environment;
  environment.set_antialias(config.antialias);

  size_t pdf_size;
  { 
    std::ifstream ifs(config.file_name, std::ifstream::in | std::ifstream::binary);
    ifs.seekg(0, std::ios::end);
    pdf_size = ifs.tellg();
    ifs.close();
  }

  pdf::Document doc(config.file_name);

  pdf::splash::Color paper_color;
  pdf::set_color(paper_color, 0xff, 0xff, 0xff);

  size_t n_pixels = 0;
  size_t djvu_pages_size = 0;
  int n_pages = doc.getNumPages();
  unsigned int page_counter = 0;
  std::auto_ptr<const Directory> output_dir;
  std::auto_ptr<File> output_file;
  std::auto_ptr<DjVm> djvm;
  std::auto_ptr<PageFiles> page_files;
  std::auto_ptr<Quantizer> quantizer;
  if (config.no_render || config.monochrome)
    quantizer.reset(new DummyQuantizer(config));
  else if (config.fg_colors > 0)
    quantizer.reset(new GraphicsMagickQuantizer(config));
  else
    quantizer.reset(new WebSafeQuantizer(config));
  if (config.format == config.FORMAT_BUNDLED)
  {
    if (config.output_stdout)
      output_file.reset(new TemporaryFile());
    else
      output_file.reset(new File(config.output));
    page_files.reset(new TemporaryPageFiles(n_pages, config.pageid_prefix));
    djvm.reset(new BundledDjVm(*output_file));
  }
  else
  {
    std::string index_file_name = "index.djvu";
    try
    {
      // For compatibility reasons, check if it's a directory.
      output_dir.reset(new Directory(config.output));
    }
    catch (OSError &no_such_directory_exception)
    {
      bool config_output_not_a_dir = false;
      try
      {
        throw;
      }
      catch (NoSuchFileOrDirectory &)
      {
        config_output_not_a_dir = true;
      }
      catch (NotADirectory &)
      {
        config_output_not_a_dir = true;
      }
      if (!config_output_not_a_dir)
        throw;
      if (config_output_not_a_dir)
      {
        // Nope, it's not a directory, it must be a file.
        std::string output_directory_name;
        split_path(config.output, output_directory_name, index_file_name);
        if (index_file_name.length() == 0)
          throw no_such_directory_exception;
        output_dir.reset(new Directory(output_directory_name));
      }
    }
    output_file.reset(new File(*output_dir, index_file_name));
    page_files.reset(new IndirectPageFiles(n_pages, *output_dir, config.pageid_prefix));
    djvm.reset(new IndirectDjVm(*output_file));
  }
  if (config.pages.size() == 0)
    config.pages.push_back(std::make_pair(1, n_pages));
  std::map<int, int> page_map;
  int opage = 1;
  for (
    std::vector< std::pair<int, int> >::iterator page_range = config.pages.begin();
    page_range != config.pages.end(); page_range++)
  for (int ipage = page_range->first; ipage <= n_pages && ipage <= page_range->second; ipage++)
  {
    page_map[ipage] = opage;
    opage++;
  }

  std::auto_ptr<pdf::Renderer> out1;
  std::auto_ptr<MutedRenderer> outm, outs;
  
  out1.reset(new pdf::Renderer(paper_color, config.monochrome));
  outm.reset(new MutedRenderer(paper_color, config.monochrome, *page_files));
  out1->startDoc(doc.getXRef());
  outm->startDoc(doc.getXRef());
  if (!config.monochrome)
  {
    outs.reset(new MutedRenderer(paper_color, config.monochrome, *page_files));
    outs->startDoc(doc.getXRef());
  }
  debug(1) << doc.getFileName()->getCString() << ":" << std::endl;
  bool crop = !config.use_media_box;
  debug(0)++;
  for (
    std::vector< std::pair<int, int> >::iterator page_range = config.pages.begin();
    page_range != config.pages.end(); page_range++)
  for (int n = page_range->first; n <= n_pages && n <= page_range->second; n++)
  {
    page_counter++;
    File &page_file = (*page_files)[n];
    debug(1) << "page #" << n << " -> #" << page_map[n];
    debug(2) << ":";
    debug(1) << std::endl;
    debug(0)++;
    debug(3) << "rendering page (1st pass)" << std::endl;
    double page_width, page_height;
    doc.get_page_size(n, crop, page_width, page_height);
    int dpi = calculate_dpi(page_width, page_height);
    doc.display_page(outm.get(), n, dpi, dpi, crop, true);
    int width = outm->getBitmapWidth();
    int height = outm->getBitmapHeight();
    n_pixels += width * height;
    debug(2) << "image size: " << width << "x" << height << std::endl;
    if (!config.no_render)
    {
      debug(3) << "rendering page (2nd pass)" << std::endl;
      doc.display_page(out1.get(), n, dpi, dpi, crop, false);
    }
    debug(3) << "preparing data for `csepdjvu`" << std::endl;
    debug(0)++;
    TemporaryFile sep_file;
    debug(3) << "storing foreground image" << std::endl;
    bool has_background = false;
    int background_color[3];
    bool has_foreground = false;
    bool has_text = false;
    (*quantizer)(out1.get(), outm.get(), width, height, background_color, has_foreground, has_background, sep_file);
    bool nonwhite_background_color;
    if (has_background)
    {
      int sub_width, sub_height;
      calculate_subsampled_size(width, height, config.bg_subsample, sub_width, sub_height);
      double hdpi = sub_width / page_width;
      double vdpi = sub_height / page_height;
      debug(3) << "rendering background image" << std::endl;
      doc.display_page(outs.get(), n, hdpi, vdpi, crop, true);
      if (sub_width != outs->getBitmapWidth())
        throw std::logic_error("Unexpected subsampled bitmap width");
      if (sub_height != outs->getBitmapHeight())
        throw std::logic_error("Unexpected subsampled bitmap height");
      pdf::Pixmap bmp(outs.get());
      debug(3) << "storing background image" << std::endl;
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
        int sub_width, sub_height;
        calculate_subsampled_size(width, height, 12, sub_width, sub_height);
        debug(3) << "storing dummy background image" << std::endl;
        sep_file << "P6 " << sub_width << " " << sub_height << " 255" << std::endl;
        for (int x = 0; x < sub_width; x++)
        for (int y = 0; y < sub_height; y++)
          sep_file.write("\xff\xff\xff", 3);
      }
    }
    if (config.text)
    {
      debug(3) << "storing text layer" << std::endl;
      const std::string &texts = outm->get_texts();
      sep_file << texts;
      has_text = texts.length() > 0;
      outm->clear_texts();
    }
    sep_file.close();
    debug(0)--;
    {
      debug(3) << "encoding layers with `csepdjvu`" << std::endl;
      DjVuCommand csepdjvu("csepdjvu");
      csepdjvu << "-d" << dpi;
      if (config.bg_slices)
        csepdjvu << "-q" << config.bg_slices;
      if (config.text == config.TEXT_LINES)
        csepdjvu << "-t";
      csepdjvu << sep_file << page_file;
      csepdjvu();
    }
    *djvm << page_file;
    TemporaryFile sjbz_file, fgbz_file, bg44_file, sed_file;
    { 
      debug(3) << "recovering images with `djvuextract`" << std::endl;
      DjVuCommand djvuextract("djvuextract");
      djvuextract << page_file;
      if (has_background || has_foreground || nonwhite_background_color)
        djvuextract
          << std::string("FGbz=") + std::string(fgbz_file)
          << std::string("BG44=") + std::string(bg44_file);
      djvuextract << std::string("Sjbz=") + std::string(sjbz_file);
      djvuextract(config.verbose < 3);
    }
    if (config.monochrome)
    {
      TemporaryFile pbm_file;
      debug(3) << "encoding monochrome image with `cjb2`" << std::endl;
      DjVuCommand cjb2("cjb2");
      cjb2 << "-losslevel" << config.loss_level << pbm_file << sjbz_file;
      pbm_file << "P4 " << width << " " << height << std::endl;
      pdf::Pixmap bmp(out1.get());
      pbm_file << bmp;
      pbm_file.close();
      cjb2();
    }
    else if (nonwhite_background_color)
    {
      TemporaryFile c44_file;
      c44_file.close();
      {
        TemporaryFile ppm_file;
        debug(3) << "creating new background image with `c44`" << std::endl;
        DjVuCommand c44("c44");
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
        debug(3) << "recovering image chunks with `djvuextract`" << std::endl;
        DjVuCommand djvuextract("djvuextract");
        djvuextract << c44_file << std::string("BG44=") + std::string(bg44_file);
        djvuextract(config.verbose < 3);
      }
    }
    {
      debug(3) << "recovering annotations with `djvused`" << std::endl;
      const std::vector<sexpr::Ref> &annotations = outm->get_annotations();
      sed_file << "select 1" << std::endl << "set-ant" << std::endl;
      for (std::vector<sexpr::Ref>::const_iterator it = annotations.begin(); it != annotations.end(); it++)
        sed_file << *it << std::endl;
      sed_file << "." << std::endl;
      outm->clear_annotations();
    }
    if (has_text)
    {
      debug(3) << "recovering text with `djvused`" << std::endl;
      DjVuCommand djvused("djvused");
      djvused << page_file << "-e" << "output-txt";
      djvused(sed_file);
    }
    sed_file.close();
    {
      debug(3) << "re-assembling page with `djvumake`" << std::endl;
      DjVuCommand djvumake("djvumake");
      std::ostringstream info;
      info << "INFO=" << width << "," << height << "," << dpi;
      djvumake
        << page_file
        << info.str()
        << std::string("Sjbz=") + std::string(sjbz_file);
      if ((has_foreground || has_background || nonwhite_background_color) && (fgbz_file.size() || bg44_file.size()))
        djvumake
          << std::string("FGbz=") + std::string(fgbz_file)
          << std::string("BG44=") + std::string(bg44_file) + std::string(":99");
      djvumake();
    }
    {
      debug(3) << "re-adding non-raster data with `djvused`" << std::endl;
      DjVuCommand djvused("djvused");
      djvused << page_file << "-s" << "-f" << sed_file;
      djvused();
    }
    {
      size_t page_size;
      page_file.reopen();
      page_size = page_file.size();
      page_file.close();
      debug(2) << page_size << " bytes out" << std::endl;
      djvu_pages_size += page_size;
    }
    debug(0)--;
  }
  if (page_counter == 0)
    throw Config::NoPagesSelected();
  djvm->create();
  if (config.extract_metadata)
  {
    TemporaryFile sed_file;
    debug(3) << "extracting XMP metadata" << std::endl;
    {
      const std::string &xmp_bytes = doc.get_xmp();
      if (xmp_bytes.length())
      {
        sexpr::GCLock gc_lock;
        static sexpr::Ref xmp_symbol = sexpr::symbol("xmp");
        sexpr::Ref xmp = sexpr::nil;
        xmp = sexpr::cons(sexpr::string(xmp_bytes), xmp);
        xmp = sexpr::cons(xmp_symbol, xmp);
        sed_file
          << "create-shared-ant" << std::endl
          << "set-ant" << std::endl
          << xmp << std::endl
          << "." << std::endl;
      }
      else
      {
        debug(0)++;
        debug(3) << "no XMP metadata" << std::endl;
        debug(0)--;
      }
    }
    debug(3) << "extracting document-information metadata" << std::endl;
    sed_file << "set-meta" << std::endl;
    pdf_metadata_to_djvu_metadata(doc, sed_file);
    sed_file << "." << std::endl;
    sed_file.close();
    djvm->set_metadata(sed_file);
  }
  if (config.extract_outline)
  {
    bool nonempty_outline;
    TemporaryFile sed_file;
    debug(3) << "extracting document outline" << std::endl;
    if (config.format == config.FORMAT_BUNDLED)
    {
      // Shared annotations chunk in necessary to preserve multi-file document structure.
      // (Single-file documents cannot contain document outline.)
      sed_file << "create-shared-ant" << std::endl;
    }
    sed_file << "set-outline" << std::endl;
    nonempty_outline = pdf_outline_to_djvu_outline(doc, sed_file, *page_files);
    sed_file << std::endl << "." << std::endl;
    sed_file.close();
    if (nonempty_outline)
      djvm->set_outline(sed_file);
  }
  djvm->commit();
  {
    size_t djvu_size = output_file->size();
    if (config.format == config.FORMAT_INDIRECT)
    {
      djvu_size += djvu_pages_size;
      try
      {
        ExistingFile shared_anno(*output_dir, "shared_anno.iff");
        djvu_size += shared_anno.size();
      }
      catch (std::ios_base::failure &ex)
      {
        // Plausibly no shared annotations
      }
    }
    double bpp = 8.0 * djvu_size / n_pixels;
    double ratio = 1.0 * pdf_size / djvu_size;
    double percent_saved = (1.0 * pdf_size - djvu_size) * 100 / pdf_size;
    debug(0)--;
    debug(1) 
      << std::fixed << std::setprecision(3) << bpp << " bits/pixel; "
      << std::fixed << std::setprecision(3) << ratio << ":1, "
      << std::fixed << std::setprecision(2) << percent_saved << "% saved, "
      << pdf_size << " bytes in, "
      << djvu_size << " bytes out" 
      << std::endl;
  }
  if (config.output_stdout)
    copy_stream(*output_file, std::cout, true);
  return 0;
}

int main(int argc, char * const argv[])
try
{
  xmain(argc, argv);
}
catch (std::ios_base::failure &ex)
{
  error_log << "I/O error (" << ex.what() << ")" << std::endl;
  exit(2);
}
catch (std::runtime_error &ex)
{
  error_log << ex << std::endl;
  exit(1);
}

// vim:ts=2 sw=2 et
