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

#include <goo/gmem.h>
#include <goo/GooString.h>
#include <GlobalParams.h>
#include <Object.h>
#include <PDFDoc.h>
#include <PDFDocEncoding.h>
#include <GfxState.h>
#include <SplashOutputDev.h>
#include <Link.h>
#include <UTF8.h>
#if POPPLER_VERSION >= 500 && POPPLER_VERSION < 509
#include <UGooString.h>
#endif

#include <splash/Splash.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashFont.h>
#include <splash/SplashGlyphBitmap.h>
#include <splash/SplashPath.h>

void init_global_params();

bool set_antialias(bool value);

PDFDoc *new_document(std::string file_name);

void set_color(SplashColor &result, uint8_t r, uint8_t g, uint8_t b);

class Renderer : public SplashOutputDev
{
public:
  Renderer(SplashColor &paper_color) :
#if POPPLER_VERSION < 500
    SplashOutputDev(splashModeRGB8Packed, gFalse, paper_color)
#else
    SplashOutputDev(splashModeRGB8, 4, gFalse, paper_color)
#endif
  { }

#if POPPLER_VERSION < 500
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
    CharCode code, Unicode *unistr, int len)
  {
    this->drawChar(state, x, y, dx, dy, origin_x, origin_y, code, -1, unistr, len);
  }

  virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y,
    CharCode code, int n_bytes, Unicode *unistr, int len)
  {
    this->SplashOutputDev::drawChar(state, x, y, dx, dy, origin_x, origin_y, code, unistr, len);
  }
  
  virtual void drawMaskedImage(GfxState *state, Object *object, Stream *stream, int width, int height,
    GfxImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height, GBool mask_invert) {}
  virtual void drawSoftMaskedImage(GfxState *state, Object *object, Stream *stream,
    int width, int height, GfxImageColorMap *color_map, Stream *mask_stream,
    int mask_width, int mask_height,	GfxImageColorMap *mask_color_map) {}

  SplashFont *getCurrentFont()
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
  void convert_path(GfxState *state, SplashPath &splash_path);
};

double get_path_area(SplashPath &path);

Object *dict_lookup(Object &dict, const char *key, Object *object);
Object *dict_lookup(Object *dict, const char *key, Object *object);
Object *dict_lookup(Dict *dict, const char *key, Object *object);

double get_page_width(PDFDoc *document, int n);
double get_page_height(PDFDoc *document, int n);

void display_page(PDFDoc *document, Renderer *renderer, int npage, double hdpi, double vdpi, bool do_links);

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

  uint8_t operator[](int n)
  {
    return this->ptr[n];
  }
};

class Pixmap
{
private:
  const uint8_t *raw_data;
  SplashBitmap *bmp;
  size_t row_size;
  int width, height;
public:
  int get_width() { return width; }
  int get_height() { return height; }

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

  PixmapIterator begin()
  {
    return PixmapIterator(raw_data, row_size);
  }

  friend std::ostream &operator<<(std::ostream &, const Pixmap &);
};

std::ostream &operator<<(std::ostream &stream, const Pixmap &pixmap);

std::string get_link_border_color(Link *link);

bool get_glyph(Splash *splash, SplashFont *font, int code, SplashGlyphBitmap *bitmap);

class XObject : public Object
{
public:
  ~XObject()
  {
    this->free();
  } 
};

#endif

// vim:ts=2 sw=2 et
