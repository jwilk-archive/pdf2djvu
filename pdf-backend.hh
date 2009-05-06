/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_PDF_BACKEND_H
#define PDF2DJVU_PDF_BACKEND_H

#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <stdint.h>

#include <GfxState.h>
#include <Link.h>
#include <PDFDoc.h>
#include <SplashOutputDev.h>

#include <splash/Splash.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashFont.h>
#include <splash/SplashGlyphBitmap.h>
#include <splash/SplashPath.h>

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

  typedef ::Stream Stream;
  typedef ::Object Object;
  typedef ::Dict Dict;
  typedef ::Catalog Catalog;
  typedef ::GooString String;
  typedef ::GBool Bool;

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
    typedef ::Link Link;
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
    Renderer(pdf::splash::Color &paper_color, bool monochrome = false) :
      pdf::splash::OutputDevice(monochrome ? splashModeMono1 : splashModeRGB8, 4, gFalse, paper_color)
    { }

    void processLink(pdf::link::Link *link, pdf::Catalog *catalog)
    {
      this->drawLink(link, catalog);
    }

    virtual void drawLink(pdf::link::Link *link, pdf::Catalog *catalog);
    virtual void drawLink(pdf::link::Link *link, const std::string &border_color, pdf::Catalog *catalog)  { }
    std::vector<std::string> link_border_colors;
  protected:
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
    Pixmap(const Pixmap&); // not defined
    Pixmap& operator=(const Pixmap&); // not defined
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
        this->byte_width = width * 4;
        break;
      }
    }

    ~Pixmap() throw ()
    {
      delete bmp;
    }

    PixmapIterator begin() const
    {
      return PixmapIterator(raw_data, row_size);
    }

    friend std::ostream &operator<<(std::ostream &, const Pixmap &);
  };


/* class pdf::OwnedObject : pdf::Object
 * ====================================
 */

  class OwnedObject : public Object
  {
  public:
    ~OwnedObject() throw ()
    {
      this->free();
    } 
  };


/* class pdf::NFKC
 * ===============
 */

  class NFKC
  {
  protected:
    Unicode* data; 
    int _length;
  public:
    explicit NFKC(Unicode *, int length);
    ~NFKC() throw ();
    size_t length() const
    {
      return static_cast<size_t>(this->_length);
    }
    operator const Unicode*() const
    {
      return this->data;
    }
  };


/* class pdf::Environment
 * ======================
 */

  class Environment
  {
  public:
    Environment();
    void set_antialias(bool value);
    class UnableToSetParameter : public std::runtime_error
    {
    public:
      UnableToSetParameter(const std::string &message)
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
    Document(const std::string &file_name);
    void display_page(Renderer *renderer, int npage, double hdpi, double vdpi, bool crop, bool do_links);
    void get_page_size(int n, bool crop, double &width, double &height);
    const std::string get_xmp();
    class LoadError : public std::runtime_error
    {
    public:
      LoadError()
      : std::runtime_error("Unable to load document")
      { }
    };
  };


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

/* Unicode → UTF-8 conversion
 * ==========================
 */

  void write_as_utf8(std::ostream &stream, Unicode unicode_char);

  std::string string_as_utf8(pdf::String *);
  std::string string_as_utf8(pdf::Object &);
}

#endif

// vim:ts=2 sw=2 et
