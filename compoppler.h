/* Copyright © 2007 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <string>
#include <fstream>

#include "goo/gmem.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "PDFDoc.h"
#include "PDFDocEncoding.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"
#include "splash/SplashFont.h"
#include "splash/SplashGlyphBitmap.h"
#include "GfxState.h"
#include "SplashOutputDev.h"
#include "Link.h"
#include "UTF8.h"

void init_global_params()
{
#if POPPLER_VERSION < 6
  globalParams = new GlobalParams(NULL);
#else
  globalParams = new GlobalParams();
#endif
}

bool set_antialias(bool value)
{
  return globalParams->setAntialias((char*)(value ? "yes" : "no"));
}

PDFDoc *new_document(std::string file_name)
{
  GooString *g_file_name = new GooString(file_name.c_str());
  PDFDoc *doc = new PDFDoc(g_file_name);
  return doc;
}

void set_color(SplashColor &result, uint8_t r, uint8_t g, uint8_t b)
{
#if POPPLER_VERSION < 5
  result.rgb8 = splashMakeRGB8(r, g, b); 
#else
  result[0] = r;
  result[1] = g;
  result[2] = b;
#endif
}

class Renderer : public SplashOutputDev
{
public:
  Renderer(SplashColor &paper_color) :
#if POPPLER_VERSION < 5
    SplashOutputDev(splashModeRGB8Packed, gFalse, paper_color)
#else
    SplashOutputDev(splashModeRGB8, 4, gFalse, paper_color)
#endif
  { }

#if POPPLER_VERSION < 5
  void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y, CharCode code, Unicode *unistr, int len)
  {
    this->drawChar(state, x, y, dx, dy, origin_x, origin_y, code, -1, unistr, len);
  }

  virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double origin_x, double origin_y, CharCode code, int n_bytes, Unicode *unistr, int len)
  {
    this->SplashOutputDev::drawChar(state, x, y, dx, dy, origin_x, origin_y, code, unistr, len);
  }
  
  virtual void drawMaskedImage(GfxState *state, Object *object, Stream *stream, int width, int height, GfxImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height, GBool mask_invert) {}
  virtual void drawSoftMaskedImage(GfxState *state, Object *object, Stream *stream, int width, int height, GfxImageColorMap *color_map, Stream *mask_stream, int mask_width, int mask_height,	GfxImageColorMap *mask_color_map) {}

  SplashFont *getCurrentFont()
  {
    return NULL;
  }
#endif  
};

Object *dict_lookup(Object &dict, const char *key, Object *object)
{
  return dict.dictLookup((char*) key, object);
}

Object *dict_lookup(Object *dict, const char *key, Object *object)
{
  return dict->dictLookup((char*) key, object);
}

Object *dict_lookup(Dict *dict, const char *key, Object *object)
{
  return dict->lookup((char*) key, object);
}

void display_page(PDFDoc *document, Renderer *renderer, int npage, double dpi, bool do_links)
{
#if POPPLER_VERSION < 5
  document->displayPage(renderer, npage, dpi, dpi, 0, gFalse, do_links);
#else    
  document->displayPage(renderer, npage, dpi, dpi, 0, gTrue, gFalse, do_links);
#endif
}

class PixmapIterator
{
private:
  const uint8_t *row_ptr;
  const uint8_t *ptr;
  int row_size;
public:  
  PixmapIterator(const uint8_t *raw_data, int row_size)
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
  int row_size;
  int width, height;
public:
  int get_width() { return width; }
  int get_height() { return height; }

  Pixmap(Renderer *renderer)
  {
#if POPPLER_VERSION < 5    
    bmp = renderer->getBitmap();
    raw_data = (const uint8_t*) bmp->getDataPtr().rgb8p;
#else
    bmp = renderer->takeBitmap();
    raw_data = (const uint8_t*) bmp->getDataPtr();
#endif
    width = bmp->getWidth();
    height = bmp->getHeight();
    row_size = bmp->getRowSize();
  }

  ~Pixmap()
  {
#if POPPLER_VERSION >= 5
    delete bmp;
#endif
  }

  PixmapIterator begin()
  {
    return PixmapIterator(raw_data, row_size);
  }

  friend std::ostream &operator<<(std::ostream &, const Pixmap &);
};

std::ostream &operator<<(std::ostream &stream, const Pixmap &pixmap)
{
  int height = pixmap.height;
  int width = pixmap.width;
  int row_size = pixmap.row_size;
  const uint8_t *row_ptr = pixmap.raw_data;
  for (int y = 0; y < height; y++)
  {
    stream.write((const char*) row_ptr, width * 3);
    row_ptr += row_size;
  }
  return stream;
}

std::string get_link_border_color(Link *link)
{
#if POPPLER_VERSION < 6
  double r, g, b;
  char buffer[8];
  LinkBorderStyle *border_style = link->getBorderStyle();
  border_style->getColor(&r, &g, &b);
  int size = snprintf(buffer, sizeof buffer, "#%02x%02x%02x", (int)(r * 0xff), (int)(g * 0xff), (int)(b * 0xff));
  return std::string(buffer, size);
#else
  static std::string red("#ff0000");
  // FIXME: find a way to determine link color
  return red;
#endif
}

class XObject : public Object
{
public:
  ~XObject()
  {
    this->free();
  } 
};

// vim:ts=2 sw=2 et
