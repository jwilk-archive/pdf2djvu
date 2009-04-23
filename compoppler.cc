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
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "compoppler.hh"
#include "debug.hh"
#include "system.hh"

#include <GlobalParams.h>
#include <PDFDocEncoding.h>
#include <UTF8.h>
#include <UnicodeTypeTable.h>

/* class pdf::Environment
 * ======================
 */

static void poppler_error_handler(int pos, char *message, va_list args)
{
  error_log << "PDF error";
  if (pos >= 0)
    error_log << " (" << pos << ")";
  error_log << ": ";
  std::ostringstream stream;
  stream_printf(stream, message, args);
  error_log << stream.str();
  error_log << std::endl;
}

pdf::Environment::Environment()
{
  globalParams = new GlobalParams();
  setErrorFunction(poppler_error_handler);
}

void pdf::Environment::set_antialias(bool value)
{
  if (!globalParams->setAntialias(const_cast<char*>(value ? "yes" : "no")))
    throw UnableToSetParameter("Unable to set antialias parameter");
  if (!globalParams->setVectorAntialias(const_cast<char*>(value ? "yes" : "no")))
    throw UnableToSetParameter("Unable to set vector antialias parameter");
}


/* class pdf::Document
 * ===================
 */

pdf::Document::Document(const std::string &file_name) 
: ::PDFDoc(new pdf::String(file_name.c_str()), NULL, NULL)
{ 
  if (!this->isOk())
    throw LoadError();
}

static std::string html_color(const double rgb[])
{
  std::ostringstream stream;
  stream << "#";
  for (int i = 0; i < 3; i++)
    stream
      << std::setw(2) << std::setfill('0') << std::hex
      << static_cast<int>(rgb[i] * 0xff);
  return stream.str();
}

static std::string html_color(double r, double g, double b)
{
  double rgb[] = {r, g, b};
  return html_color(rgb);
}

static void cmyk_to_rgb(double cmyk[], double rgb[])
{
  static pdf::gfx::DeviceCmykColorSpace cmyk_space;
  pdf::gfx::Color cmyk_cc;
  pdf::gfx::RgbColor rgb_cc;
  for (int i = 0; i < 4; i++)
    cmyk_cc.c[i] = pdf::gfx::double_as_color_component(cmyk[i]);
  cmyk_space.getRGB(&cmyk_cc, &rgb_cc);
  rgb[0] = pdf::gfx::color_component_as_double(rgb_cc.r);
  rgb[1] = pdf::gfx::color_component_as_double(rgb_cc.g);
  rgb[2] = pdf::gfx::color_component_as_double(rgb_cc.b);
}

static GBool annotations_callback(pdf::ant::Annotation *annotation, void *user_data)
{
  std::vector<std::string> &border_colors = *reinterpret_cast<std::vector<std::string>*>(user_data);
  std::string border_color;
  if (annotation->getType() != pdf::ant::Annotation::typeLink)
    return true;
  pdf::ant::Color *color = annotation->getColor();
  if (color == NULL)
  {
    border_colors.push_back("");
    return true;
  }
  double *values = color->getValues();
  switch (color->getSpace())
  {
  case pdf::ant::Color::colorTransparent:
    break;
  case pdf::ant::Color::colorGray:
    border_color = html_color(values[0], values[0], values[0]);
    break;
  case pdf::ant::Color::colorRGB:
    border_color = html_color(values);
    break;
  case pdf::ant::Color::colorCMYK:
    {
      double rgb[3];
      cmyk_to_rgb(values, rgb);
      border_color = html_color(rgb);
    }
  }
  border_colors.push_back(border_color);
  return true;
}

void pdf::Document::display_page(pdf::Renderer *renderer, int npage, double hdpi, double vdpi, bool crop, bool do_links)
{
  renderer->link_border_colors.clear();
  this->displayPage(renderer, npage, hdpi, vdpi, 0, !crop, crop, !do_links, 
    NULL, NULL, 
    do_links ? annotations_callback : NULL,
    do_links ? &renderer->link_border_colors : NULL
  );
  std::reverse(renderer->link_border_colors.begin(), renderer->link_border_colors.end());
  this->processLinks(renderer, npage);
}

void pdf::Document::get_page_size(int n, bool crop, double &width, double &height)
{
  width = crop ?
      this->getPageCropWidth(n) :
      this->getPageMediaWidth(n);
  height = crop ?
      this->getPageCropHeight(n) :
      this->getPageMediaHeight(n);
  width /= 72.0;
  height /= 72.0;
  if ((this->getPageRotate(n) / 90) & 1)
    std::swap(width, height);
}

const std::string pdf::Document::get_xmp()
{
  std::auto_ptr<pdf::String> mstring;
  mstring.reset(this->readMetadata());
  if (mstring.get() == NULL)
    return "";
  const char *cstring = mstring->getCString();
  if (strncmp(cstring, "<?xpacket begin=", 16) != 0)
    return "";
  cstring += 16;
  while (*cstring && *cstring != '?')
    cstring++;
  if (*cstring++ != '?' || *cstring++ != '>')
    return "";
  while (*cstring && *cstring != '<')
    cstring++;
  const char *endcstring = strrchr(cstring, '>');
  if (endcstring < cstring + 32)
    return "";
  if (*--endcstring != '?')
    return "";
  char quote = *--endcstring;
  if ((quote != '\'') && (quote != '"'))
    return "";
  if (*--endcstring != 'w' || *--endcstring != quote)
    return "";
  endcstring -= 14;
  if (strncmp(endcstring, "<?xpacket end=", 14) != 0)
    return "";
  while (endcstring > cstring && *endcstring != '>')
    endcstring--;
  return std::string(cstring, endcstring - cstring + 1);
}


/* utility functions
 * =================
 */

void pdf::set_color(splash::Color &result, uint8_t r, uint8_t g, uint8_t b)
{
  result[0] = r;
  result[1] = g;
  result[2] = b;
}


/* class pdf::Renderer : pdf::splash::OutputDevice
 * ===============================================
 */

void pdf::Renderer::drawLink(pdf::link::Link *link, pdf::Catalog *catalog)
{
  std::string border_color;
  if (this->link_border_colors.size())
  {
    border_color = this->link_border_colors.back();
    this->link_border_colors.pop_back();
  }
  this->drawLink(link, border_color, catalog);
}


/* glyph-related functions
 * =======================
 */

bool pdf::get_glyph(splash::Splash *splash, splash::Font *font, 
  double x, double y, int code, splash::GlyphBitmap *bitmap)
{
  if (font == NULL)
    return false;
  splash::ClipResult clip_result;
  if (!font->getGlyph(code, 0, 0, bitmap, static_cast<int>(x), static_cast<int>(y), splash->getClip(), &clip_result))
    return false;
  return (clip_result != splashClipAllOutside);
}


/* path-related functions/methods
 * ==============================
 */

void pdf::Renderer::convert_path(pdf::gfx::State *state, splash::Path &splash_path)
{
  // copied from <PopplerOutputDev.h>
  pdf::gfx::Subpath *subpath;
  pdf::gfx::Path *path = state->getPath();
  int n_subpaths = path->getNumSubpaths();
  for (int i = 0; i < n_subpaths; i++) 
  {
    subpath = path->getSubpath(i);
    if (subpath->getNumPoints() > 0) 
    {
      double x1, y1, x2, y2, x3, y3;
      state->transform(subpath->getX(0), subpath->getY(0), &x1, &y1);
      splash_path.moveTo((splash::Coord)x1, (splash::Coord)y1);
      int j = 1;
      int n_points = subpath->getNumPoints();
      while (j < n_points)
      {
        if (subpath->getCurve(j)) 
        {
          state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
          state->transform(subpath->getX(j + 1), subpath->getY(j + 1), &x2, &y2);
          state->transform(subpath->getX(j + 2), subpath->getY(j + 2), &x3, &y3);
          splash_path.curveTo(
            (splash::Coord)x1, (splash::Coord)y1,
            (splash::Coord)x2, (splash::Coord)y2,
            (splash::Coord)x3, (splash::Coord)y3
          );
          j += 3;
        } 
        else 
        {
          state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
          splash_path.lineTo((splash::Coord)x1, (splash::Coord)y1);
          j++;
        }
      }
      if (subpath->isClosed())
        splash_path.close();
    }
  }
}

double pdf::get_path_area(splash::Path &path)
{
  double area = 0.0;
  int path_len = path.getLength();
  double x0, y0;
  Guchar ch;
  path.getPoint(0, &x0, &y0, &ch);
  for (int i = 0; i < path_len - 1; i++)
  {
    double x1, y1, x2, y2;
    path.getPoint(i + 1, &x1, &y1, &ch);
    path.getPoint((i + 2) % path_len, &x2, &y2, &ch);
    x1 -= x0; y1 -= y0;
    x2 -= x0; y2 -= y0;
    area += x1 * y2 - x2 * y1;
  }
  return fabs(area);
}


/* dictionary lookup
 * =================
 */

pdf::Object *pdf::dict_lookup(pdf::Object &dict, const char *key, pdf::Object *object)
{
  return dict.dictLookup(const_cast<char*>(key), object);
}

pdf::Object *pdf::dict_lookup(pdf::Object *dict, const char *key, pdf::Object *object)
{
  return dict->dictLookup(const_cast<char*>(key), object);
}

pdf::Object *pdf::dict_lookup(pdf::Dict *dict, const char *key, pdf::Object *object)
{
  return dict->lookup(const_cast<char*>(key), object);
}


/* Unicode → UTF-8 conversion
 * ==========================
 */

void pdf::write_as_utf8(std::ostream &stream, Unicode unicode_char)
{
  char buffer[8];
  int seqlen = mapUTF8(unicode_char, buffer, sizeof buffer);
  stream.write(buffer, seqlen);
}

std::string pdf::string_as_utf8(pdf::String *string)
{
  /* See
   * http://unicode.org/faq/utf_bom.html
   * for description of both UTF-16 and UTF-8.
   */
  const static uint32_t replacement_character = 0xfffd;
  const char *cstring = string->getCString();
  size_t clength = string->getLength();
  std::ostringstream stream;
  if (clength >= 2 && (cstring[0] & 0xff) == 0xfe && (cstring[1] & 0xff) == 0xff)
  {
    /* UTF-16-BE Byte Order Mark has been recognized. */
    uint32_t code, code_shift = 0;
    for (size_t i = 2; i < clength; i += 2)
    {
      if (i + 1 < clength)
      {
        code = ((cstring[i] & 0xff) << 8) + (cstring[i + 1] & 0xff);
        fprintf(stderr, "< %04x\n", code);
      }
      else
      {
        /* A lone byte is encountered. */
        code = replacement_character;
      }
      if (code_shift)
      {
        /* A leading surrogate has been encoutered. */
        if (code >= 0xdc00 && code < 0xe000)
        {
          /* A trailing surrogate is encountered. */
          code = code_shift + (code & 0x3ff);
          if (code >= 0x110000)
            code = replacement_character;
        }
        else
        {
          /* An unparied surrogate is encoutered. */ 
          code = replacement_character;
        }
        code_shift = 0;
      }
      else if (code >= 0xd800 && code < 0xdc00)
      {
        /* A leading surrogate is encountered. */
        code_shift = 0x10000 + ((code & 0x3ff) << 10);
        continue;
      }
      if ((code & 0xfffe) == 0xfffe)
      {
        /* A non-character is encountered. */
        code = replacement_character;
      }
      if (code < 0x80)
      {
        char ascii = code;
        stream << ascii;
      }
      else
      {
        char buffer[4];
        size_t nbytes;
        for (nbytes = 2; nbytes < 4; nbytes++)
          if (code < (1 << (5 * nbytes + 1)))
            break;
        buffer[0] = (0xff00 >> nbytes) & 0xff;
        for (size_t i = nbytes - 1; i; i--)
        {
          buffer[i] = 0x80 | (code & 0x3f);
          code >>= 6;
        }
        buffer[0] |= code;
        stream.write(buffer, nbytes);
      }
    }
  }
  else
  {
    /* Use the PDFDoc encoding. */
    for (size_t i = 0; i < clength; i++)
      write_as_utf8(stream, pdfDocEncoding[cstring[i] & 0xff]);
  }
  return stream.str();
}

std::string pdf::string_as_utf8(pdf::Object &object)
{
  return pdf::string_as_utf8(object.getString());
}


/* class pdf::Pixmap
 * =================
 */

namespace pdf
{
  std::ostream &operator<<(std::ostream &stream, const pdf::Pixmap &pixmap)
  {
    const uint8_t *row_ptr = pixmap.raw_data;
    if (pixmap.monochrome)
    {
      for (int y = 0; y < pixmap.height; y++)
      {
        for (size_t x = 0; x < pixmap.byte_width; x++)
          stream.put(static_cast<char>(row_ptr[x] ^ 0xff));
        row_ptr += pixmap.row_size;
      }
    }
    else
    {
      for (int y = 0; y < pixmap.height; y++)
      {
        stream.write(reinterpret_cast<const char*>(row_ptr), pixmap.byte_width);
        row_ptr += pixmap.row_size;
      }
    }
    return stream;
  }
}


/* class pdf::NFKC
 * ===============
 */

pdf::NFKC::NFKC(Unicode *unistr, int length) 
: data(NULL), _length(0)
{
  data = unicodeNormalizeNFKC(unistr, length, &this->_length, NULL);
}

pdf::NFKC::~NFKC() throw ()
{
  gfree(this->data);
}

// vim:ts=2 sw=2 et
