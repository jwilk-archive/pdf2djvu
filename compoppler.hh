/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_COMPOPPLER_H
#define PDF2DJVU_COMPOPPLER_H

#include <string>
#include <ostream>

#include <PDFDoc.h>
#include <GfxState.h>
#include <SplashOutputDev.h>
#include <Link.h>

#include <splash/Splash.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashFont.h>
#include <splash/SplashGlyphBitmap.h>
#include <splash/SplashPath.h>

namespace splash
{
  typedef ::Splash Splash;
  typedef ::SplashColor Color;
  typedef ::SplashFont Font;
  typedef ::SplashCoord Coord;
  typedef ::SplashPath Path;
  typedef ::SplashClipResult ClipResult;
  typedef ::SplashGlyphBitmap GlyphBitmap;
  typedef ::SplashBitmap Bitmap;
  typedef ::SplashOutputDev OutputDevice;
}


namespace pdf 
{

/* type definitions
 * ================
 */

  typedef ::PDFDoc Document;
  typedef ::Stream Stream;
  typedef ::Object Object;
  typedef ::Dict Dict;
  typedef ::Catalog Catalog;
  typedef ::GooString String;

  namespace link
  {
    typedef ::Link Link;
    typedef ::LinkAction Action;
    typedef ::LinkDest Destination;
    typedef ::LinkGoTo GoTo;
    typedef ::LinkURI URI;
#if POPPLER_VERSION < 509
    typedef ::LinkBorderStyle BorderStyle; 
#endif
  }

  namespace gfx
  {
    typedef ::GfxSubpath Subpath;
    typedef ::GfxPath Path;
    typedef ::GfxState State;
    typedef ::GfxImageColorMap ImageColorMap;
  }

/* class pdf::Renderer : splash::OutputDevice
 * ==========================================
 */

  class Renderer : public splash::OutputDevice
  {
  public:
    Renderer(splash::Color &paper_color) :
#if POPPLER_VERSION < 500
      splash::OutputDevice(splashModeRGB8Packed, gFalse, paper_color)
#else
      splash::OutputDevice(splashModeRGB8, 4, gFalse, paper_color)
#endif
    { }

#if POPPLER_VERSION < 500
    void drawChar(gfx::State *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
      CharCode code, Unicode *unistr, int len)
    {
      this->drawChar(state, x, y, dx, dy, origin_x, origin_y, code, -1, unistr, len);
    }

    virtual void drawChar(gfx::State *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
      CharCode code, int n_bytes, Unicode *unistr, int len)
    {
      this->splash::OutputDevice::drawChar(state, x, y, dx, dy, origin_x, origin_y, code, unistr, len);
    }
    
    virtual void drawMaskedImage(gfx::State *state, Object *object, Stream *stream, int width, int height,
      gfx::ImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height, GBool mask_invert) {}
    virtual void drawSoftMaskedImage(gfx::State *state, Object *object, Stream *stream,
      int width, int height, gfx::ImageColorMap *color_map, Stream *mask_stream,
      int mask_width, int mask_height,	gfx::ImageColorMap *mask_color_map) {}

    splash::Font *getCurrentFont()
    {
      return NULL;
    }
#endif  

#if POPPLER_VERSION >= 600
    void processLink(Link *link, Catalog *catalog)
    {
      this->drawLink(link, catalog);
    }

    virtual void drawLink(Link *link, Catalog *catalog) { }
#endif

  protected:
    static void convert_path(gfx::State *state, splash::Path &splash_path);
  };


/* class pdf::Pixmap::iterator
 * ===========================
 */

  class PixmapIterator
  {
  private:
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
    const uint8_t *raw_data;
    splash::Bitmap *bmp;
    size_t row_size;
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

    Pixmap(Renderer *renderer)
    {
#if POPPLER_VERSION < 500
      bmp = renderer->getBitmap();
      raw_data = const_cast<const uint8_t*>(bmp->getDataPtr().rgb8p);
#else
      bmp = renderer->takeBitmap();
      raw_data = const_cast<const uint8_t*>(bmp->getDataPtr());
#endif
      width = bmp->getWidth();
      height = bmp->getHeight();
      row_size = bmp->getRowSize();
    }
    
    ~Pixmap()
    {
#if POPPLER_VERSION >= 500
      delete bmp;
#endif
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
    ~OwnedObject()
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
    ~NFKC();
    size_t length() const
    {
      return static_cast<size_t>(this->_length);
    }
    operator const Unicode*() const
    {
      return this->data;
    }
  };


/* global poppler options
 * ======================
 */

  void init_global_params();
  bool set_antialias(bool value);

/* utility functions
 * =================
 */

  pdf::Document *new_document(std::string file_name);
  void display_page(pdf::Document *document, Renderer *renderer, int npage, double hdpi, double vdpi, bool crop, bool do_links);
  void set_color(splash::Color &result, uint8_t r, uint8_t g, uint8_t b);
  std::string get_link_border_color(Link *link);

/* glyph-related functions
 * =======================
 */

  bool get_glyph(splash::Splash *splash, splash::Font *font, int code, splash::GlyphBitmap *bitmap);

/* dictionary lookup
 * =================
 */

  Object *dict_lookup(Object &dict, const char *key, Object *object);
  Object *dict_lookup(Object *dict, const char *key, Object *object);
  Object *dict_lookup(Dict *dict, const char *key, Object *object);

/* page width and height
 * =====================
 */

  double get_page_width(pdf::Document *document, int n, bool crop);
  double get_page_height(pdf::Document *document, int n, bool crop);

/* path-related functions
 * ======================
 */

  double get_path_area(splash::Path &path);

/* Unicode → UTF-8 conversion
 * ==========================
 */

  void write_as_utf8(std::ostream &stream, Unicode unicode_char);
  void write_as_utf8(std::ostream &stream, char pdf_char);
  void write_as_utf8(std::ostream &stream, const char *pdf_chars);
}

#endif

// vim:ts=2 sw=2 et
