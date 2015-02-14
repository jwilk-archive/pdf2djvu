/* Copyright © 2007, 2008, 2009, 2010 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>
#if _OPENMP
#include <omp.h>
#endif

#include "pdf-backend.hh"
#include "pdf-dpi.hh"
#include "config.hh"
#include "debug.hh"
#include "djvuconst.hh"
#include "paths.hh"
#include "image-filter.hh"
#include "sexpr.hh"
#include "system.hh"
#include "version.hh"
#include "i18n.hh"
#include "xmp.hh"

Config config;

static inline DebugStream &debug(int n)
{
  return debug(n, config.verbose);
}

class NoLinkDestination : public std::runtime_error
{
public:
  NoLinkDestination()
  : std::runtime_error(_("Cannot find link destination"))
  { }
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

class PageMap : protected std::map<int, int>
{
protected:
  int max;
public:
  PageMap()
  {
    this->max = std::numeric_limits<int>::min();
  }

  int get_max() const
  {
    return this->max;
  }

  int get(int n) const
  {
    const_iterator it = this->find(n);
    if (it == this->end())
      throw std::logic_error(_("Page not found"));
    return it->second;
  }

  int get(int n, int m) const
  {
    const_iterator it = this->find(n);
    if (it == this->end())
      return m;
    return it->second;
  }

  void set(int n, int m)
  {
    if (m > this->max)
      this->max = m;
    (*this)[n] = m;
  }
};

class DocumentMap
{
protected:
  intmax_t byte_size;
  const std::vector<const char *> &filenames;
  std::vector<int> map;
public:
  DocumentMap(const std::vector<const char *> &filenames_)
  : byte_size(0),
    filenames(filenames_)
  {
    int n_all_pages = 0;
    for (std::vector<const char*>::iterator it = config.filenames.begin(); it != config.filenames.end(); it++)
    {
      {
        std::ifstream ifs(*it, std::ifstream::in | std::ifstream::binary);
        ifs.seekg(0, std::ios::end);
        this->byte_size += ifs.tellg();
        ifs.close();
      }
      {
        std::auto_ptr<pdf::Document> doc(new pdf::Document(*it));
        int n_pages = doc->getNumPages();
        this->map.push_back(n_all_pages);
        n_all_pages += n_pages;
      }
    }
    this->map.push_back(n_all_pages);
  }

  intmax_t get_byte_size(void)
  {
    return this->byte_size;
  }

  int get_n_pages(void)
  {
    return this->map.back();
  }

  void get(int n, const char *&filename, int &m)
  {
    assert(n >= 1);
    size_t i = std::upper_bound(this->map.begin(), this->map.end(), n - 1) - this->map.begin() - 1;
    filename = this->filenames.at(i);
    m = n - this->map[i];
  }
};

class Component
{
protected:
  std::auto_ptr<std::string> title;
  File *file;
public:
  explicit Component(File &file, const std::string *title = NULL)
  : file(&file)
  {
    if (title != NULL)
      this->title.reset(new std::string(*title));
    file.close();
  }

  Component(const Component &component)
  : file(component.file)
  {
    if (component.title.get() != NULL)
      this->title.reset(new std::string(*component.title));
  }

  const std::string * get_title() const
  {
    return this->title.get();
  }

  const std::string & get_basename() const
  {
    return this->file->get_basename();
  }

  size_t size()
  {
    size_t result;
    this->file->reopen();
    result = this->file->size();
    this->file->close();
    return result;
  }

  friend std::ostream &operator <<(std::ostream &, const Component &);
  friend Command &operator <<(Command &, const Component &);
};

Command &operator <<(Command &command, const Component &component)
{
  command << *component.file;
  return command;
}

std::ostream &operator <<(std::ostream &stream, const Component &component)
{
  stream << *component.file;
  return stream;
}

class ComponentList
{
protected:
  std::vector<File*> files;
  std::vector<Component*> components;
  const PageMap &page_map;

  ComponentList(int n, const PageMap &page_map)
  : files(n), components(n), page_map(page_map)
  { }

  void clean_files()
  {
    for (std::vector<Component*>::iterator it = this->components.begin(); it != this->components.end(); it++)
    {
      delete *it;
      *it = NULL;
    }
    for (std::vector<File*>::iterator it = this->files.begin(); it != this->files.end(); it++)
    {
      delete *it;
      *it = NULL;
    }
  }

  string_format::Bindings get_bindings(int n) const
  {
    string_format::Bindings bindings;
    bindings["max_spage"] = this->files.size();
    bindings["spage"] = n;
    bindings["max_page"] = this->files.size();
    bindings["page"] = n;
    bindings["max_dpage"] = this->page_map.get_max();
    bindings["dpage"] = this->page_map.get(n, 0);
    return bindings;
  }

  std::string *get_title(int n) const
  {
    if (config.page_title_template.get() == NULL)
      return NULL;
    string_format::Bindings bindings = this->get_bindings(n);
    return new std::string(config.page_title_template->format(bindings));
  }

  virtual File *create_file(const std::string &id)
  = 0;

public:
  virtual Component &operator[](int n)
  {
    std::vector<Component*>::reference tmpfile_ptr = this->components.at(n - 1);
    if (tmpfile_ptr == NULL)
    {
      this->files[n - 1] = this->create_file(this->get_file_name(n));
      tmpfile_ptr = new Component(*this->files[n - 1], this->get_title(n));
    }
    return *tmpfile_ptr;
  }

  std::string get_file_name(int n) const
  {
    string_format::Bindings bindings = this->get_bindings(n);
    return config.pageid_template->format(bindings);
  }

  virtual ~ComponentList() throw ()
  {
    this->clean_files();
  }
};

typedef pdf::Renderer MainRenderer;

class MutedRenderer: public pdf::Renderer
{
protected:
  std::auto_ptr<std::ostringstream> text_comments;
  std::vector<sexpr::Ref> annotations;
  const ComponentList &page_files;
  bool skipped_elements;

  void add_text_comment(int ox, int oy, int dx, int dy, int x, int y, int w, int h, const Unicode *unistr, int len)
  {
    while (len > 0 && *unistr == ' ')
      unistr++, len--;
    if (len == 0)
      return;
    *(this->text_comments)
      << std::dec << std::noshowpos
      << "\x01 \x02 " /* special characters will be replaced later */
      << ox << ":" << oy << " "
      << dx << ":" << dy << " "
      <<  w << "\x03" <<  h << std::showpos << x << y << " "
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
  pdf::Bool needNonText()
  {
    return !config.no_render;
  }

  void drawImageMask(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::Bool invert, pdf::Bool interpolate, pdf::Bool inline_image)
  {
    this->skipped_elements = true;
    return;
  }

  void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate, int *mask_colors, pdf::Bool inline_image)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
    {
      this->skipped_elements = true;
      return;
    }
    Renderer::drawImage(state, object, stream, width, height, color_map,
        interpolate, mask_colors, inline_image);
  }

  void drawMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height, pdf::Bool mask_invert, pdf::Bool mask_interpolate)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
    {
      this->skipped_elements = true;
      return;
    }
    Renderer::drawMaskedImage(state, object, stream, width, height,
      color_map, interpolate,
      mask_stream, mask_width, mask_height, mask_invert, mask_interpolate);
  }

  void drawSoftMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream,
    int width, int height, pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height,
    pdf::gfx::ImageColorMap *mask_color_map, pdf::Bool mask_interpolate)
  {
    if (is_foreground_color_map(color_map) || config.no_render)
    {
      this->skipped_elements = true;
      return;
    }
    Renderer::drawSoftMaskedImage(state, object, stream, width, height,
      color_map, interpolate,
      mask_stream, mask_width, mask_height, mask_color_map, mask_interpolate);
  }

  pdf::Bool interpretType3Chars() { return false; }

  void drawChar(pdf::gfx::State *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
    CharCode code, int n_bytes, Unicode *unistr, int length)
  {
    double pox, poy, pdx, pdy, px, py, pw, ph;
    x -= origin_x; y -= origin_y;
    state->transform(x, y, &pox, &poy);
    state->transformDelta(dx, dy, &pdx, &pdy);
    int old_render = state->getRender();
    /* Setting the following rendering mode disallows drawing text, but allows
     * fonts to be set up properly nevertheless:
     */
    state->setRender(0x103);
    this->skipped_elements = true;
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
      /* Ideally, this should never happen. Some heuristics is required to
       * determine character width/height: */
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
    if (config.text_crop)
    {
      int bitmap_width = this->getBitmapWidth();
      int bitmap_height = this->getBitmapHeight();
      if (px + pw < 0 || py + ph < 0 || px >= bitmap_width || py >= bitmap_height)
        return;
    }
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

  void draw_link(pdf::link::Link *link, const std::string &border_color)
  {
    sexpr::GCLock gc_lock;
    if (!config.hyperlinks.extract)
      return;
    double x1, y1, x2, y2;
    pdf::link::Action *link_action = link->getAction();
    if (link_action == NULL)
    {
      debug(1) << _("Warning: Unable to convert link without an action") << std::endl;
      return;
    }
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
        page = get_page_for_LinkGoTo(dynamic_cast<pdf::link::GoTo*>(link_action), this->catalog);
      }
      catch (NoLinkDestination &ex)
      {
        debug(1) << string_printf(_("Warning: %s"), ex.what()) << std::endl;
        return;
      }
      std::ostringstream strstream;
      strstream << "#" << this->page_files.get_file_name(page);
      uri = strstream.str();
      break;
    }
    case actionGoToR:
      debug(1) << _("Warning: Unable to convert link with a remote go-to action") << std::endl;
      return;
    case actionNamed:
      debug(1) << _("Warning: Unable to convert link with a named action") << std::endl;
      return;
    default:
      debug(1) << _("Warning: Unknown link action") << std::endl;
      return;
    }
    int x, y, w, h;
    this->cvtUserToDev(x1, y1, &x, &y);
    this->cvtUserToDev(x2, y2, &w, &h);
    w -= x;
    h = y - h;
    y = this->getBitmapHeight() - y;
    static sexpr::Ref symbol_xor = sexpr::symbol("xor");
    static sexpr::Ref symbol_border = sexpr::symbol("border");
    static sexpr::Ref symbol_rect = sexpr::symbol("rect");
    static sexpr::Ref symbol_maparea = sexpr::symbol("maparea");
    sexpr::Ref expr = sexpr::nil;
    if (config.hyperlinks.border_always_visible)
    {
      static sexpr::Ref symbol_border_avis = sexpr::symbol("border_avis");
      sexpr::Ref item = sexpr::cons(sexpr::symbol("border_avis"), sexpr::nil);
      expr = sexpr::cons(item, expr);
    }
    if (config.hyperlinks.border_color.length() > 0)
    {
      static sexpr::Ref symbol_border = sexpr::symbol("border");
      sexpr::Ref item = sexpr::cons(sexpr::symbol(config.hyperlinks.border_color), sexpr::nil);
      item = sexpr::cons(symbol_border, item);
      expr = sexpr::cons(item, expr);
    }
    else
    {
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

  pdf::Bool useDrawChar()
  {
    return true;
  }

  void stroke(pdf::gfx::State *state)
  {
    this->skipped_elements = true;
  }

  void fill(pdf::gfx::State *state)
  {
    if (config.no_render)
    {
      this->skipped_elements = true;
      return;
    }
    pdf::splash::Path path;
    this->convert_path(state, path);
    double area = pdf::get_path_area(path);
    if (area / this->getBitmapHeight() / this->getBitmapWidth() >= 0.8)
      Renderer::fill(state);
  }

  void eoFill(pdf::gfx::State *state)
  {
    this->fill(state);
  }

  MutedRenderer(pdf::splash::Color &paper_color, bool monochrome, const ComponentList &page_files)
  : Renderer(paper_color, monochrome), page_files(page_files)
  {
    this->clear();
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
    std::string texts = this->text_comments->str();
    if (config.text_filter_command_line.length() > 0)
      texts = Command::filter(config.text_filter_command_line, texts);
    for (std::string::iterator it = texts.begin(); it != texts.end(); it++)
      switch (*it)
      {
      case '\x01':
        *it = '#'; break;
      case '\x02':
        *it = 'T'; break;
      case '\x03':
        *it = 'x'; break;
      }
    return texts;
  }

  void clear_texts()
  {
    this->text_comments.reset(new std::ostringstream);
    *(this->text_comments) << std::setfill('0');
  }

  void clear()
  {
    this->skipped_elements = 0;
    this->clear_texts();
    this->clear_annotations();
  }

  bool has_skipped_elements()
  {
    return this->skipped_elements;
  }
};

class BookmarkError : public std::runtime_error
{
public:
  BookmarkError(const std::string &message)
  : std::runtime_error(message)
  { }
};

class NoPageForBookmark : public BookmarkError
{
public:
  NoPageForBookmark()
  : BookmarkError(_("No page for a bookmark"))
  { }
};

class NoTitleForBookmark : public BookmarkError
{
public:
  NoTitleForBookmark()
  : BookmarkError(_("No title for a bookmark"))
  { }
};

static sexpr::Expr pdf_outline_to_djvu_outline(pdf::Object *node, pdf::Catalog *catalog,
  const ComponentList &page_files)
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
      debug(1) << string_printf(_("Warning: %s"), ex.what()) << std::endl;
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
  const ComponentList &page_files)
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

static void add_meta_string(const char *key, const std::string &value, std::ostream &stream)
{
  sexpr::Ref expr = sexpr::string(value);
  stream << key << "\t" << expr << std::endl;
}

static void add_meta_date(const char *key, const pdf::Timestamp &value, std::ostream &stream)
{
  try
  {
    sexpr::Ref sexpr = sexpr::string(value.format(' '));
    stream << key << "\t" << sexpr << std::endl;
  }
  catch (pdf::Timestamp::Invalid)
  {
    debug(1) << string_printf(_("Warning: metadata[%s] is not a valid date"), key) << std::endl;
  }
}

static void pdf_metadata_to_djvu_metadata(pdf::Metadata &metadata, std::ostream &stream)
{
  sexpr::GCLock gc_lock;
  metadata.iterate<std::ostream>(add_meta_string, add_meta_date, stream);
}

class TemporaryComponentList : public ComponentList
{
private:
  TemporaryComponentList(const TemporaryComponentList&); // not defined
  TemporaryComponentList& operator=(const TemporaryComponentList&); // not defined
protected:
  std::auto_ptr<const TemporaryDirectory> directory;
  std::auto_ptr<const TemporaryFile> shared_ant_file;

  virtual File *create_file(const std::string &pageid)
  {
    return new TemporaryFile(*this->directory, pageid);
  }
public:
  explicit TemporaryComponentList(int n, const PageMap &page_map)
  : ComponentList(n, page_map),
    directory(new TemporaryDirectory()),
    shared_ant_file(new TemporaryFile(*directory, djvu::shared_ant_file_name))
  { }

  virtual ~TemporaryComponentList() throw ()
  {
    this->clean_files();
  }
};

class IndirectComponentList : public ComponentList
{
private:
  IndirectComponentList(const IndirectComponentList&); // not defined
  IndirectComponentList& operator=(const IndirectComponentList&); // not defined
protected:
  const Directory &directory;
  virtual File *create_file(const std::string &pageid)
  {
    return new File(this->directory, pageid);
  }
public:
  IndirectComponentList(int n, const PageMap &page_map, const Directory &directory)
  : ComponentList(n, page_map), directory(directory)
  { }
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
  DjVuCommand::dir_name = absolute_path(paths::djvulibre_bindir, argv0_dir_name);
}

class DjVm
{
protected:
  std::set<std::string> known_ids;
  std::set<std::string> known_titles;
  class DuplicateId : public std::runtime_error
  {
  public:
    DuplicateId(const std::string &id)
    : std::runtime_error(string_printf(_("Duplicate page identifier: %s"), id.c_str()))
    { }
  };
  class DuplicateTitle : public std::runtime_error
  {
  public:
    DuplicateTitle(const std::string &id)
    : std::runtime_error(string_printf(_("Duplicate page title: %s"), id.c_str()))
    { }
  };
  void remember(const Component &component);
public:
  virtual void add(const Component &compontent) = 0;
  virtual void create() = 0;
  virtual void commit() = 0;
  DjVm &operator <<(const Component &component)
  {
    this->add(component);
    return *this;
  }
  virtual void set_outline(File &outline_sed_file) = 0;
  virtual void set_metadata(File &metadata_sed_file) = 0;
  virtual ~DjVm() throw () { /* just to shut up compilers */ }
};

void DjVm::remember(const Component &component)
{
  std::string id;
  id = component.get_basename();
  if (this->known_ids.count(id) > 0)
    throw DuplicateId(id);
  this->known_ids.insert(id);
  const std::string *title = component.get_title();
  if (title != NULL)
  {
    if (this->known_titles.count(*title) > 0)
      throw DuplicateTitle(*title);
    this->known_titles.insert(*title);
  }
}

class IndirectDjVm;

class BundledDjVm : public DjVm
{
protected:
  File &output_file;
  DjVuCommand converter;
  std::auto_ptr<IndirectDjVm> indirect_djvm;
  std::auto_ptr<TemporaryFile> index_file;
public:
  explicit BundledDjVm(File &output_file)
  : output_file(output_file),
    converter("djvmcvt")
  { }
  ~BundledDjVm() throw ()
  { }
  virtual void add(const Component &component);
  virtual void set_outline(File &outlines_sed_file);
  virtual void set_metadata(File &metadata_sed_file);
  virtual void create();
  virtual void commit();
};

class IndirectDjVm : public DjVm
{
protected:
  File &index_file;
  std::vector<Component> components;
  class UnexpectedDjvuSedOutput : public std::runtime_error
  {
  public:
    UnexpectedDjvuSedOutput()
    : std::runtime_error(_("Unexpected output from djvused"))
    { };
  };
  void do_create(const std::vector<Component> &components, bool shared_ant = false);
public:
  explicit IndirectDjVm(File &index_file)
  : index_file(index_file)
  { }

  virtual ~IndirectDjVm() throw ()
  { }

  virtual void add(const Component &component)
  {
    this->remember(component);
    this->components.push_back(component);
  }

  virtual void set_outline(File &outlines_sed_file)
  {
    debug(3) << _("creating document outline with `djvused`") << std::endl;
    DjVuCommand djvused("djvused");
    djvused << "-s" << "-f" << outlines_sed_file << this->index_file;
    djvused(); // djvused -s -f <outlines-sed-file> <index-djvu-file>
  }

  virtual void set_metadata(File &metadata_sed_file)
  {
    size_t size = this->components.size();
    if (size <= 2)
    {
      debug(3) << _("setting metadata with `djvused`") << std::endl;
      DjVuCommand djvused("djvused");
      djvused << "-s" << "-f" << metadata_sed_file << this->index_file;
      djvused(); // djvused -s -f <metadata-sed-file> <index-djvu-file>
    }
    else
    {
      /* Using ``djvused`` to add shared annotations to a indirect multi-page
       * document could be unacceptably slow:
       * http://sourceforge.net/p/djvu/bugs/100/
       *
       * We need to work around this bug.
       */
      debug(3) << _("setting metadata with `djvused` (working around a DjVuLibre bug)") << std::endl;
      std::vector<Component> dummy_components;
      TemporaryFile dummy_sed_file;
      /* For an indirect document, ``create-shared-ant`` is, surprisingly, not
       * enough to create a shared annotations chunk. The dummy shared
       * annotation is actually required.
       */
      dummy_sed_file
        << "create-shared-ant" << std::endl
        << "set-ant" << std::endl
        << "(x)" << std::endl
        << "." << std::endl;
      dummy_sed_file.close();
      for (size_t i = 0; i < size - 1; i++)
      {
        /* To make the page include the annotations chunk:
         * - Create an ephemeral indirect multi-page document with just this
         *   single page.
         * - Add the dummy shared annotation.
         */
        dummy_components.push_back(this->components[i]);
        this->do_create(dummy_components);
        DjVuCommand djvused("djvused");
        djvused << "-s" << "-f" << dummy_sed_file << this->index_file;
        djvused(); // djvused -s -f <dummy-sed-file> <output-djvu-file>
        dummy_components.pop_back();
      }
      /* For the last page, use the desired annotations. */
      dummy_components.push_back(this->components[size - 1]);
      this->do_create(dummy_components);
      DjVuCommand djvused("djvused");
      djvused << "-s" << "-f" << metadata_sed_file << this->index_file;
      djvused(); // djvused -s -f <metadata-sed-file> <output-djvu-file>
      /* Finally, recreate the index. */
      this->do_create(this->components, true /* include shared annotations */);
    }
  }

  virtual void create()
  {
    size_t size = this->components.size();
    debug(3)
      << string_printf(ngettext(
           "creating multi-page indirect document (%d page)",
           "creating multi-page indirect document (%d pages)",
           size), size
         )
      << std::endl;
    this->do_create(this->components);
  }

  virtual void commit()
  { }
};

void BundledDjVm::add(const Component &component)
{
  if (this->index_file.get() == NULL)
  {
    std::ostringstream stream;
    stream << component << ".djvu-index";
    this->index_file.reset(new TemporaryFile(stream.str()));
    this->indirect_djvm.reset(new IndirectDjVm(*this->index_file));
  }
  /* No need to call ``this->remember(component)``. */
  *this->indirect_djvm << component;
}

void BundledDjVm::set_outline(File &outlines_sed_file)
{
  this->indirect_djvm->set_outline(outlines_sed_file);
}

void BundledDjVm::set_metadata(File &metadata_sed_file)
{
  this->indirect_djvm->set_metadata(metadata_sed_file);
}

void BundledDjVm::create()
{
  this->indirect_djvm->create();
}

void BundledDjVm::commit()
{
  this->converter
    << "-b"
    << *this->index_file
    << this->output_file;
  this->converter(); // djvmcvt -b <output-djvu-file> <index-djvu-file>
  this->index_file.reset(NULL);
}

void IndirectDjVm::do_create(const std::vector<Component> &components, bool shared_ant)
{
  size_t size = components.size();
  this->index_file.reopen(true); // (re)open and truncate
  this->index_file.write(djvu::binary::djvm_head, sizeof djvu::binary::djvm_head);
  this->index_file << djvu::binary::version;
  for (int i = 1; i >= 0; i--)
    index_file << static_cast<char>(((size + shared_ant) >> (8 * i)) & 0xff);
  {
    TemporaryFile bzz_file;
    for (size_t i = 0; i < size + shared_ant; i++)
      bzz_file.write("\0\0", 3);
    if (shared_ant)
      bzz_file << '\3';
    for (std::vector<Component>::const_iterator it = components.begin(); it != components.end(); it++)
      bzz_file << (it->get_title() == NULL ? '\001' : '\101');
    if (shared_ant)
      bzz_file << djvu::shared_ant_file_name << '\0';
    for (std::vector<Component>::const_iterator it = components.begin(); it != components.end(); it++)
    {
      bzz_file << it->get_basename() << '\0';
      const std::string *title = it->get_title();
      if (title != NULL)
        bzz_file << encoding::proxy<encoding::native, encoding::utf8>(*title) << '\0';
    }
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

static int calculate_dpi(const pdf::dpi::Guess &guess)
{
  double dpi = guess.max() + 0.5;
  if (dpi < djvu::min_dpi)
    return djvu::min_dpi;
  else if (dpi > djvu::max_dpi)
    return djvu::max_dpi;
  else
    return static_cast<int>(dpi);
}

static int calculate_dpi(pdf::Document &doc, int n, bool crop)
{
  double page_width, page_height;
  doc.get_page_size(n, crop, page_width, page_height);
  if (config.guess_dpi)
  {
    pdf::dpi::Guesser dpi_guesser(doc);
    try
    {
      pdf::dpi::Guess guess = dpi_guesser[n];
      std::ostringstream guess_str;
      guess_str << guess;
      debug(2)
        << string_printf(_("guessed resolution: %s dpi"), guess_str.str().c_str())
        << std::endl;
      return calculate_dpi(guess);
    }
    catch (pdf::dpi::NoGuess)
    {
      debug(2) << _("unable to guess resolution") << std::endl;
    }
  }
  if (config.preferred_page_size.first)
  {
    double hdpi = config.preferred_page_size.first / page_width;
    double vdpi = config.preferred_page_size.second / page_height;
    double dpi = std::min(hdpi, vdpi) + 0.5;
    int int_dpi;
    if (dpi < djvu::min_dpi)
      int_dpi = djvu::min_dpi;
    else if (dpi > djvu::max_dpi)
      int_dpi = djvu::max_dpi;
    else
      int_dpi = static_cast<int>(dpi);
      debug(2)
        << string_printf(_("estimated resolution: %d dpi"), int_dpi)
        << std::endl;
    return int_dpi;
  }
  else
    return config.dpi;
}

class StdoutIsATerminal : public std::runtime_error
{
public:
  StdoutIsATerminal()
  : std::runtime_error(_("I won't write DjVu data to a terminal."))
  { }
};

static void calculate_subsampled_size(int width, int height, int ratio, int &sub_width, int &sub_height)
{
  /* DjVuLibre expects that:
   *
   *   sub_width = ceil(width / ratio)
   *   sub_width = ceil(height / ratio)
   *
   * However, DjVu Reference (10.3) requires that:
   *
   *   ceil(width / sub_width) = ceil(height / sub_height)
   *
   * This functions satisfies all these equations by decreasing ratio if
   * necessary.
   *
   * See http://sourceforge.net/p/djvu/bugs/106/ for details.
   */
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
    if (argc <= 1)
      prevent_pop_out();
    exit(1);
  }

  if (config.output_stdout && is_stream_a_tty(std::cout))
    throw StdoutIsATerminal();

  pdf::Environment environment(argv[0]);
  environment.set_antialias(config.antialias);

  DocumentMap document_map(config.filenames);
  intmax_t pdf_byte_size = document_map.get_byte_size();

  pdf::splash::Color paper_color;
  pdf::set_color(paper_color, 0xff, 0xff, 0xff);

  intmax_t n_pixels = 0;
  intmax_t djvu_pages_size = 0;
  int n_pages = document_map.get_n_pages();
  PageMap page_map;
  std::vector<int> page_numbers;
  std::auto_ptr<const Directory> output_dir;
  std::auto_ptr<File> output_file;
  std::auto_ptr<DjVm> djvm;
  std::auto_ptr<ComponentList> page_files;
  std::auto_ptr<Quantizer> quantizer;
  if (config.monochrome)
    quantizer.reset(new DummyQuantizer(config));
  else
    switch (config.fg_colors)
    {
    case Config::FG_COLORS_DEFAULT:
      quantizer.reset(new DefaultQuantizer(config));
      break;
    case Config::FG_COLORS_WEB:
      quantizer.reset(new WebSafeQuantizer(config));
      break;
    case Config::FG_COLORS_BLACK:
      quantizer.reset(new MaskQuantizer(config));
      break;
    default:
      quantizer.reset(new GraphicsMagickQuantizer(config));
    }

#if _OPENMP
  if (config.n_jobs >= 1)
    omp_set_num_threads(config.n_jobs);
#else
  if (config.n_jobs != 1)
  {
    debug(1) << string_printf(_("Warning: %s"), _("pdf2djvu was built without OpenMP support; multi-threading is disabled.")) << std::endl;
    config.n_jobs = 1;
  }
#endif

  if (config.format == config.FORMAT_BUNDLED)
  {
    if (config.output_stdout)
      output_file.reset(new TemporaryFile());
    else
      output_file.reset(new File(config.output));
    page_files.reset(new TemporaryComponentList(n_pages, page_map));
    djvm.reset(new BundledDjVm(*output_file));
  }
  else
  {
    std::string index_file_name = "index.djvu";
    try
    {
      /* For compatibility reasons, check if it's a directory: */
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
        /* No, it's not a directory, it must be a file: */
        std::string output_directory_name;
        split_path(config.output, output_directory_name, index_file_name);
        if (index_file_name.length() == 0)
          throw no_such_directory_exception;
        output_dir.reset(new Directory(output_directory_name));
      }
    }
    output_file.reset(new File(*output_dir, index_file_name));
    page_files.reset(new IndirectComponentList(n_pages, page_map, *output_dir));
    djvm.reset(new IndirectDjVm(*output_file));
  }
  if (config.pages.size() == 0)
    config.pages.push_back(std::make_pair(1, n_pages));
  for (
    std::vector< std::pair<int, int> >::iterator page_range = config.pages.begin();
    page_range != config.pages.end(); page_range++)
  for (int n = page_range->first; n <= n_pages && n <= page_range->second; n++)
  {
    static int i = 1;
    page_map.set(n, i);
    page_numbers.push_back(n);
    i++;
  }

  if (page_numbers.size() == 0)
    throw Config::NoPagesSelected();

  std::auto_ptr<MainRenderer> out1;
  std::auto_ptr<MutedRenderer> outm, outs;
  std::auto_ptr<pdf::Document> doc;
  const char *doc_filename = NULL;

  bool crop = !config.use_media_box;
  debug(0)++;
  #pragma omp parallel for private(out1, outm, outs, doc) firstprivate(doc_filename) reduction(+: djvu_pages_size) schedule(runtime)
  for (size_t i = 0; i < page_numbers.size(); i++)
  try
  {
    int m, n = page_numbers[i];
    const char *new_filename;
    document_map.get(n, new_filename, m);
    if (new_filename != doc_filename)
    {
      doc_filename = new_filename;
      doc.reset(new pdf::Document(doc_filename));
      #pragma omp critical
      {
        debug(0)--;
        debug(1) << doc->getFileName()->getCString() << ":" << std::endl;
        debug(0)++;
      }
      out1.reset(new MainRenderer(paper_color, config.monochrome));
      out1->start_doc(doc.get());
      outm.reset(new MutedRenderer(paper_color, config.monochrome, *page_files));
      outm->start_doc(doc.get());
      if (!config.monochrome)
      {
        outs.reset(new MutedRenderer(paper_color, config.monochrome, *page_files));
        outs->start_doc(doc.get());
      }
    }
    assert(doc.get() != NULL);
    assert(out1.get() != NULL);
    assert(outm.get() != NULL);
    if (!config.monochrome)
      assert(outs.get() != NULL);
    Component &component = (*page_files)[n];
    #pragma omp critical
    {
      debug(1) << string_printf(_("page #%d -> #%d"), n, page_map.get(n));
      if (config.n_jobs != 1)
        debug(2) << ":";
      debug(1) << std::endl;
    }
#if _OPENMP
/* Multi-threading would interact badly with logging. Disable it for now. */
#define debug(x) if (config.n_jobs == 1) (debug)(x)
#endif
    debug(0)++;
    debug(3) << _("rendering page (1st pass)") << std::endl;
    double page_width, page_height;
    doc->get_page_size(m, crop, page_width, page_height);
    int dpi = calculate_dpi(*doc, m, crop);
    doc->display_page(outm.get(), m, dpi, dpi, crop, true);
    int width = outm->getBitmapWidth();
    int height = outm->getBitmapHeight();
    n_pixels += width * height;
    debug(2) << string_printf(_("image size: %dx%d"), width, height) << std::endl;
    if (!config.no_render && outm->has_skipped_elements())
    { /* Render the page second time, without skipping any elements. */
      debug(3) << _("rendering page (2nd pass)") << std::endl;
      doc->display_page(out1.get(), m, dpi, dpi, crop, false);
    }
    debug(3) << _("preparing data for `csepdjvu`") << std::endl;
    debug(0)++;
    TemporaryFile sep_file;
    debug(3) << _("storing foreground image") << std::endl;
    bool has_background = false;
    int background_color[3];
    bool has_foreground = false;
    bool has_text = false;
    (*quantizer)(
        outm->has_skipped_elements()
        ? static_cast<pdf::Renderer*>(out1.get())
        : static_cast<pdf::Renderer*>(outm.get()),
        outm.get(),
        width, height,
        background_color, has_foreground, has_background,
        sep_file
    );
    bool nonwhite_background_color;
    if (has_background)
    {
      /* The image has a real (non-solid) background. Store subsampled IW44 image. */
      int sub_width, sub_height;
      calculate_subsampled_size(width, height, config.bg_subsample, sub_width, sub_height);
      double hdpi = sub_width / page_width;
      double vdpi = sub_height / page_height;
      debug(3) << _("rendering background image") << std::endl;
      doc->display_page(outs.get(), m, hdpi, vdpi, crop, true);
      if (sub_width != outs->getBitmapWidth())
        throw std::logic_error(_("Unexpected subsampled bitmap width"));
      if (sub_height != outs->getBitmapHeight())
        throw std::logic_error(_("Unexpected subsampled bitmap height"));
      pdf::Pixmap bmp(outs.get());
      debug(3) << _("storing background image") << std::endl;
      sep_file << "P6 " << sub_width << " " << sub_height << " 255" << std::endl;
      sep_file << bmp;
      nonwhite_background_color = false;
      outs->clear();
    }
    else
    {
      /* Background is solid. */
      nonwhite_background_color = (background_color[0] & background_color[1] & background_color[2] & 0xff) != 0xff;
      if (nonwhite_background_color)
      { /* Create a dummy background, just to assure existence of FGbz chunks.
         * The background chunk will be replaced later: */
        int sub_width, sub_height;
        calculate_subsampled_size(width, height, 12, sub_width, sub_height);
        debug(3) << _("storing dummy background image") << std::endl;
        sep_file << "P6 " << sub_width << " " << sub_height << " 255" << std::endl;
        for (int x = 0; x < sub_width; x++)
        for (int y = 0; y < sub_height; y++)
          sep_file.write("\xff\xff\xff", 3);
      }
    }
    if (config.text)
    {
      debug(3) << _("storing text layer") << std::endl;
      const std::string &texts = outm->get_texts();
      sep_file << texts;
      has_text = texts.length() > 0;
      outm->clear_texts();
    }
    sep_file.close();
    debug(0)--;
    {
      debug(3) << _("encoding layers with `csepdjvu`") << std::endl;
      DjVuCommand csepdjvu("csepdjvu");
      csepdjvu << "-d" << dpi;
      if (config.bg_slices)
        csepdjvu << "-q" << config.bg_slices;
      if (config.text == config.TEXT_LINES)
        csepdjvu << "-t";
      csepdjvu << sep_file << component;
      csepdjvu();
    }
    const bool should_have_fgbz = has_background || has_foreground || nonwhite_background_color;
    const bool need_reassemble =
      config.no_render
      ? false
      : (config.monochrome || nonwhite_background_color || !should_have_fgbz);
    TemporaryFile sed_file;
    if (need_reassemble)
    {
      TemporaryFile sjbz_file, fgbz_file, bg44_file;
      if (!config.monochrome)
      { /* Extract FGbz and BG44 image chunks, to that they can be mangled and
         * re-assembled later: */
        debug(3) << _("recovering images with `djvuextract`") << std::endl;
        DjVuCommand djvuextract("djvuextract");
        djvuextract << component;
        if (should_have_fgbz)
          djvuextract
            << std::string("FGbz=") + std::string(fgbz_file)
            << std::string("BG44=") + std::string(bg44_file);
        djvuextract << std::string("Sjbz=") + std::string(sjbz_file);
        djvuextract(config.verbose < 3);
      }
      if (config.monochrome)
      { /* Use cjb2 for lossy compression: */
        TemporaryFile pbm_file;
        debug(3) << _("encoding monochrome image with `cjb2`") << std::endl;
        DjVuCommand cjb2("cjb2");
        cjb2 << "-losslevel" << config.loss_level << pbm_file << sjbz_file;
        pbm_file << "P4 " << width << " " << height << std::endl;
        pdf::Pixmap bmp(
          outm->has_skipped_elements()
          ? static_cast<pdf::Renderer*>(out1.get())
          : static_cast<pdf::Renderer*>(outm.get())
        );
        pbm_file << bmp;
        pbm_file.close();
        cjb2();
      }
      else if (nonwhite_background_color)
      {
        TemporaryFile c44_file;
        c44_file.close();
        { /* Create solid-color PPM image with subsample ratio 12: */
          TemporaryFile ppm_file;
          debug(3) << _("creating new background image with `c44`") << std::endl;
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
        { /* Replace previous (dummy) BG44 chunk with the newly created one: */
          debug(3) << _("recovering image chunks with `djvuextract`") << std::endl;
          DjVuCommand djvuextract("djvuextract");
          djvuextract << c44_file << std::string("BG44=") + std::string(bg44_file);
          djvuextract(config.verbose < 3);
        }
      }
      if (has_text)
      { /* Extract hidden text layer (as created by csepdjvu); save it into the sed file: */
        debug(3) << _("recovering text with `djvused`") << std::endl;
        DjVuCommand djvused("djvused");
        djvused << component << "-e" << "output-txt";
        djvused(sed_file);
      }
      { /* Re-assemble new DjVu using previously mangled chunks: */
        debug(3) << _("re-assembling page with `djvumake`") << std::endl;
        DjVuCommand djvumake("djvumake");
        std::ostringstream info;
        info << "INFO=" << width << "," << height << "," << dpi;
        djvumake
          << component
          << info.str()
          << std::string("Sjbz=") + std::string(sjbz_file);
        if (should_have_fgbz && (fgbz_file.size() || bg44_file.size()))
          djvumake
            << std::string("FGbz=") + std::string(fgbz_file)
            << std::string("BG44=") + std::string(bg44_file) + std::string(":99");
        djvumake();
      }
    }
    { /* Extract annotations (hyperlinks); save it into the sed file: */
      debug(3) << _("extracting annotations") << std::endl;
      const std::vector<sexpr::Ref> &annotations = outm->get_annotations();
      sed_file << "select 1" << std::endl << "set-ant" << std::endl;
      for (std::vector<sexpr::Ref>::const_iterator it = annotations.begin(); it != annotations.end(); it++)
        sed_file << *it << std::endl;
      sed_file << "." << std::endl;
      outm->clear_annotations();
    }
    outm->clear();
    sed_file.close();
    { /* Add per-page non-raster data into the DjVu file: */
      debug(3) << _("adding non-raster data with `djvused`") << std::endl;
      DjVuCommand djvused("djvused");
      djvused << component << "-s" << "-f" << sed_file;
      djvused();
    }
    {
      size_t page_size = component.size();
      debug(2)
        << string_printf(ngettext("%zu bytes out", "%zu bytes out", page_size), page_size)
        << std::endl;
      djvu_pages_size += page_size;
    }
    debug(0)--;
#if _OPENMP
#undef debug
#endif
  }
  /* These exception handlers duplicate the ones in main(), for the sake of OMP.
   * They should be kept in sync.
   */
  catch (std::ios_base::failure &ex)
  {
    error_log << string_printf(_("I/O error (%s)"), ex.what()) << std::endl;
    exit(2);
  }
  catch (std::runtime_error &ex)
  {
    error_log << ex << std::endl;
    exit(1);
  }
  /* Separate loop to avoid parallelization: */
  for (size_t i = 0; i < page_numbers.size(); i++)
  {
    int n = page_numbers[i];
    *djvm << (*page_files)[n];
  }
  djvm->create();
  /* Only first PDF document metadata/outline is taken into account. */
  doc.reset(new pdf::Document(config.filenames[0]));
  if (config.extract_metadata)
  {
    TemporaryFile sed_file;
    pdf::Metadata metadata(*doc);
    debug(3) << _("extracting XMP metadata") << std::endl;
    {
      std::string xmp_bytes = doc->get_xmp();
      debug(0)++;
      if (config.adjust_metadata)
        try
        {
          xmp_bytes = xmp::transform(xmp_bytes, metadata);
        }
        catch (xmp::XmlError &ex)
        {
          debug(1) << string_printf(_("Warning: %s"), ex.what()) << std::endl;
        }
      debug(0)--;
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
    }
    debug(3) << _("extracting document-information metadata") << std::endl;
    sed_file << "set-meta" << std::endl;
    pdf_metadata_to_djvu_metadata(metadata, sed_file);
    sed_file << "." << std::endl;
    sed_file.close();
    djvm->set_metadata(sed_file);
  }
  if (config.extract_outline)
  {
    bool nonempty_outline;
    TemporaryFile sed_file;
    debug(3) << _("extracting document outline") << std::endl;
    if (config.format == config.FORMAT_BUNDLED)
    {
      /* Shared annotations chunk in necessary to preserve multi-file document
       * structure. (Single-file documents cannot contain document outline.) */
      sed_file << "create-shared-ant" << std::endl;
    }
    sed_file << "set-outline" << std::endl;
    nonempty_outline = pdf_outline_to_djvu_outline(*doc, sed_file, *page_files);
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
        ExistingFile shared_anno(*output_dir, djvu::shared_ant_file_name);
        djvu_size += shared_anno.size();
      }
      catch (std::ios_base::failure &ex)
      {
        /* Let's assume that there are no shared annotations. */
      }
    }
    /* Poppler resets the LC_NUMERIC locale settings:
     * https://bugs.debian.org/533425
     *
     * We need to work around this bug.
     */
    i18n::setup_locale();
    double bpp = 8.0 * djvu_size / n_pixels;
    double ratio = 1.0 * pdf_byte_size / djvu_size;
    double percent_saved = (1.0 * pdf_byte_size - djvu_size) * 100 / pdf_byte_size;
    debug(0)--;
    debug(1)
      << string_printf(
           _("%.3f bits/pixel; %.3f:1, %.2f%% saved, %ju bytes in, %zu bytes out"),
           bpp, ratio, percent_saved, pdf_byte_size, djvu_size
         )
      << std::endl;
  }
  if (config.output_stdout)
    copy_stream(*output_file, std::cout, true);
  return 0;
}

int main(int argc, char * const argv[])
try
{
  i18n::setup(argv[0]);
  xmain(argc, argv);
}
/* Please keep the exception handlers in sync with the ones in xmain(). */
catch (std::ios_base::failure &ex)
{
  error_log << string_printf(_("I/O error (%s)"), ex.what()) << std::endl;
  exit(2);
}
catch (std::runtime_error &ex)
{
  error_log << ex << std::endl;
  exit(1);
}

// vim:ts=2 sts=2 sw=2 et
