/* Copyright © 2007-2022 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
 *
 * This file is part of pdf2djvu.
 *
 * pdf2djvu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * pdf2djvu is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef PDF2DJVU_PDF_BACKEND_H
#define PDF2DJVU_PDF_BACKEND_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "autoconf.hh"

// Poppler:
#include <PDFDoc.h>
#include <Annot.h>
#include <Catalog.h>
#include <Dict.h>
#include <GfxState.h>
#include <Link.h>
#include <Object.h>
#include <OutputDev.h>
#include <SplashOutputDev.h>
#include <Stream.h>
#include <goo/GooString.h>
#include <goo/gfile.h>
#include <splash/Splash.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashClip.h>
#include <splash/SplashFont.h>
#include <splash/SplashGlyphBitmap.h>
#include <splash/SplashPath.h>
#include <splash/SplashTypes.h>

#include "i18n.hh"

namespace pdf
{

/* type definitions — splash output device
 * =======================================
 */

  namespace splash
  {
    typedef ::Splash Splash;
    typedef ::SplashColor Color;
    typedef ::SplashFont Font;
    typedef ::SplashCoord Coord;
    typedef ::SplashPath Path;
    typedef ::SplashGlyphBitmap GlyphBitmap;
    typedef ::SplashBitmap Bitmap;
    typedef ::SplashOutputDev OutputDevice;
    typedef ::SplashClipResult ClipResult;
  }

/* miscellaneous type definitions
 * ==============================
 */

  typedef ::OutputDev OutputDevice;
  typedef ::Stream Stream;
  typedef ::Object Object;
  typedef ::Dict Dict;
  typedef ::Catalog Catalog;
  typedef ::GooString String;
  typedef ::Goffset Offset;
  typedef ::Ref Ref;

/* type definitions — annotations
 * ==============================
 */

  namespace ant
  {
    typedef ::Annot Annotation;
    typedef ::AnnotColor Color;
  }

/* type definitions — hyperlinks
 * =============================
 */

  namespace link
  {
    typedef ::AnnotLink Link;
    typedef ::LinkAction Action;
    typedef ::LinkDest Destination;
    typedef ::LinkGoTo GoTo;
    typedef ::LinkURI URI;
  }

/* type definitions — rendering subsystem
 * ======================================
 */

  namespace gfx
  {
    typedef ::GfxSubpath Subpath;
    typedef ::GfxPath Path;
    typedef ::GfxState State;
    typedef ::GfxImageColorMap ImageColorMap;
    typedef ::GfxColorComp ColorComponent;
    typedef ::GfxColor Color;
    typedef ::GfxRGB RgbColor;
    typedef ::GfxDeviceCMYKColorSpace DeviceCmykColorSpace;
  }

/* class pdf::Renderer : pdf::splash::OutputDevice
 * ===============================================
 */

  class Renderer : public pdf::splash::OutputDevice
  {
  public:
    explicit Renderer(pdf::splash::Color &paper_color, bool monochrome = false);

    void processLink(pdf::link::Link *link)
    {
      this->drawLink(link, nullptr);
    }
    virtual void drawLink(pdf::link::Link *link, pdf::Catalog *catalog);
    virtual void draw_link(pdf::link::Link *link, const std::string &border_color)
    { }
    std::vector<std::string> link_border_colors;
    void start_doc(::PDFDoc *doc)
    {
      this->startDoc(doc);
      this->catalog = doc->getCatalog();
    }
  protected:
    pdf::Catalog *catalog;
    static void convert_path(gfx::State *state, pdf::splash::Path &splash_path);
  };


/* class pdf::Pixmap::iterator
 * ===========================
 */

  class PixmapIterator
  {
  protected:
    const uint8_t *row_ptr;
    const uint8_t *ptr;
    size_t row_size;
  public:
    PixmapIterator(const uint8_t *raw_data, size_t row_size)
    {
      this->row_ptr = this->ptr = raw_data;
      this->row_size = row_size;
    }

    PixmapIterator &operator ++(int)
    {
      ptr += 3;
      return *this;
    }

    void next_row()
    {
      ptr = row_ptr = row_ptr + row_size;
    }

    uint8_t operator[](int n) const
    {
      return this->ptr[n];
    }
  };


/* class pdf::Pixmap
 * =================
 */

  class Pixmap
  {
  private:
    Pixmap(const Pixmap&) = delete;
    Pixmap& operator=(const Pixmap&) = delete;
  protected:
    const uint8_t *raw_data;
    pdf::splash::Bitmap *bmp;
    size_t row_size;
    size_t byte_width;
    bool monochrome;
    int width, height;
  public:
    typedef PixmapIterator iterator;

    int get_width() const
    {
      return width;
    }
    int get_height() const
    {
      return height;
    }

    explicit Pixmap(Renderer *renderer)
    {
      bmp = renderer->takeBitmap();
      raw_data = const_cast<const uint8_t*>(bmp->getDataPtr());
      width = bmp->getWidth();
      height = bmp->getHeight();
      row_size = bmp->getRowSize();
      this->monochrome = false;
      switch (bmp->getMode())
      {
      case splashModeMono1:
        this->byte_width = (width + 7) / 8;
        this->monochrome = true;
        break;
      case splashModeMono8:
        this->byte_width = width;
        break;
      case splashModeRGB8:
      case splashModeBGR8:
        this->byte_width = width * 3;
        break;
      case splashModeXBGR8:
#if POPPLER_VERSION >= 8100 || defined(SPLASH_CMYK)
      case splashModeCMYK8:
#endif
        this->byte_width = width * 4;
        break;
#if POPPLER_VERSION >= 8100 || defined(SPLASH_CMYK)
      case splashModeDeviceN8:
#endif
      default:
        assert(0 && "unexpected splash mode");
      }
    }

    ~Pixmap()
    {
      delete bmp;
    }

    PixmapIterator begin() const
    {
      return PixmapIterator(raw_data, row_size);
    }

    friend std::ostream &operator<<(std::ostream &, const Pixmap &);
  };


/* class pdf::Environment
 * ======================
 */

  class Environment
  {
  public:
    Environment();
    static bool antialias;
    void set_antialias(bool value);
    class UnableToSetParameter : public std::runtime_error
    {
    public:
      explicit UnableToSetParameter(const std::string &message)
      : std::runtime_error(message)
      { }
    };
  };


/* class pdf::Document
 * ===================
 */

  class Document : public ::PDFDoc
  {
  public:
    explicit Document(const std::string &file_name);
    void display_page(Renderer *renderer, int npage, double hdpi, double vdpi, bool crop, bool do_links);
    void get_page_size(int n, bool crop, double &width, double &height);
    const std::string get_xmp();
    void get_doc_info(pdf::Object &info)
    {
#if POPPLER_VERSION < 5800
      this->getDocInfo(&info);
#else
      info = this->getDocInfo();
#endif
    }
    class LoadError : public std::runtime_error
    {
    public:
      LoadError()
      : std::runtime_error(_("Unable to load document"))
      { }
    };
  };


/* class pdf::Timestamp
 * ====================
 */

  class Timestamp
  {
  protected:
    bool dummy;
    struct tm timestamp;
    char tz_sign;
    int tz_hour;
    int tz_minute;
  public:
    Timestamp();
    Timestamp(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, char tz_sign = 0, int tz_hour = 0, int tz_minute = 0);
    std::string format(char separator = 'T') const;
    static Timestamp now();

    class Invalid : public std::runtime_error
    {
    public:
      Invalid()
      : std::runtime_error(_("Invalid date format"))
      { }
    };
  };


/* class pdf::Metadata
 * ===================
 */

  class Metadata
  {
  protected:
    typedef std::pair<const char *, std::string *> string_field;
    std::vector<string_field> string_fields;
    typedef std::pair<const char *, pdf::Timestamp *> date_field;
    std::vector<date_field> date_fields;
  public:
    explicit Metadata(pdf::Document &document);
    std::string title;
    std::string subject;
    std::string keywords;
    std::string author;
    std::string creator;
    std::string producer;
    Timestamp creation_date;
    Timestamp mod_date;
    template <typename T>
    void iterate(
      void (*string_callback)(const char *, const std::string &, T &),
      void (*date_callback)(const char *, const Timestamp &, T &),
      T &extra
    ) const;
  };

  template <typename T>
  void Metadata::iterate(
    void (*string_callback)(const char *, const std::string &, T &),
    void (*date_callback)(const char *, const Timestamp &, T &),
    T &extra
  ) const
  {
    for (const auto &field : this->string_fields)
      string_callback(field.first, *field.second, extra);
    for (const auto &field : this->date_fields)
      date_callback(field.first, *field.second, extra);
  }


/* utility functions
 * =================
 */

  void set_color(pdf::splash::Color &result, uint8_t r, uint8_t g, uint8_t b);

  namespace gfx
  {
    static inline double color_component_as_double(pdf::gfx::ColorComponent c)
    {
      return ::colToDbl(c);
    }

    static inline pdf::gfx::ColorComponent double_as_color_component(double x)
    {
      return ::dblToCol(x);
    }
  }

/* glyph-related functions
 * =======================
 */

  bool get_glyph(pdf::splash::Splash *splash, pdf::splash::Font *font,
    double x, double y, // x, y are transformed (i.e. output device) coordinates
    int code, pdf::splash::GlyphBitmap *bitmap);

/* dictionary lookup
 * =================
 */

  pdf::Object *dict_lookup(pdf::Object &dict, const char *key, pdf::Object *object);
  pdf::Object *dict_lookup(pdf::Object *dict, const char *key, pdf::Object *object);
  pdf::Object *dict_lookup(pdf::Dict *dict, const char *key, pdf::Object *object);

/* path-related functions
 * ======================
 */

  double get_path_area(pdf::splash::Path &path);

  const char * get_c_string(const pdf::String *str);

  int find_page(pdf::Catalog *catalog, pdf::Ref pgref);

}

#endif

// vim:ts=2 sts=2 sw=2 et
