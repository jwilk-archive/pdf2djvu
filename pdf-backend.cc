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

#include "pdf-backend.hh"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits.h>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include <Error.h>
#include <GfxState.h>
#include <GlobalParams.h>
#include <PDFDoc.h>
#include <goo/GooString.h>
#include <goo/gfile.h>
#include <splash/SplashClip.h>
#include <splash/SplashTypes.h>

#include "debug.hh"
#include "i18n.hh"
#include "pdf-unicode.hh"
#include "string-printf.hh"
#include "sys-time.hh"
#include "system.hh"


/* class pdf::Environment
 * ======================
 */

#if POPPLER_VERSION >= 8500
static void poppler_error_handler(ErrorCategory category, pdf::Offset pos, const char *message)
#else
static void poppler_error_handler(void *data, ErrorCategory category, pdf::Offset pos, const char *message)
#endif
{
  const char *category_name = _("PDF error");
  switch (category)
  {
    case errSyntaxWarning:
      category_name = _("PDF syntax warning");
      break;
    case errSyntaxError:
      category_name = _("PDF syntax error");
      break;
    case errConfig:
      category_name = _("Poppler configuration error");
      break;
    case errCommandLine:
      break; /* should not happen */
    case errIO:
      category_name = _("Input/output error");
      break;
    case errNotAllowed:
      category_name = _("Permission denied");
      break;
    case errUnimplemented:
      category_name = _("PDF feature not implemented");
      break;
    case errInternal:
      category_name = _("Internal Poppler error");
      break;
  }

  if (pos >= 0)
  {
    error_log <<
      /* L10N: "<error-category> (<position>): <error-message>" */
      string_printf(_("%s (%jd): %s"), category_name, static_cast<intmax_t>(pos), message);
  }
  else
  {
    error_log <<
      /* L10N: "<error-category>: <error-message>" */
      string_printf(_("%s: %s"), category_name, message);
  }
  error_log << std::endl;
}

#if POPPLER_VERSION < 7000
static void poppler_error_handler(void *data, ErrorCategory category, pdf::Offset pos, char *message)
{
  poppler_error_handler(data, category, pos, const_cast<const char *>(message));
}
#endif

pdf::Environment::Environment()
{
#if POPPLER_VERSION >= 8300
  globalParams = std::unique_ptr<GlobalParams>(new GlobalParams);
#else
  globalParams = new GlobalParams;
#endif
#if POPPLER_VERSION >= 8500
  setErrorCallback(poppler_error_handler);
#else
  setErrorCallback(poppler_error_handler, nullptr);
#endif
}

void pdf::Environment::set_antialias(bool value)
{
  this->antialias = value;
}


/* class pdf::Document
 * ===================
 */

pdf::Document::Document(const std::string &file_name)
#if POPPLER_VERSION >= 220300
: ::PDFDoc(std::make_unique<pdf::String>(file_name.c_str()))
#else
: ::PDFDoc(new pdf::String(file_name.c_str()))
#endif
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
      << static_cast<int>(rgb[i] * 0xFF);
  return stream.str();
}

static std::string html_color(double r, double g, double b)
{
  double rgb[] = {r, g, b};
  return html_color(rgb);
}

static void cmyk_to_rgb(const double cmyk[], double rgb[])
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

static bool annotations_callback(pdf::ant::Annotation *annotation, void *user_data)
{
  std::vector<std::string> &border_colors = *reinterpret_cast<std::vector<std::string>*>(user_data);
  std::string border_color;
  if (annotation->getType() != pdf::ant::Annotation::typeLink)
    return true;
  pdf::ant::Color *color = annotation->getColor();
  if (color == nullptr)
  {
    border_colors.push_back("");
    return true;
  }
  const double *values = color->getValues();
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
    nullptr, nullptr,
    do_links ? annotations_callback : nullptr,
    do_links ? &renderer->link_border_colors : nullptr
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
  std::unique_ptr<const pdf::String> mstring;
#if POPPLER_VERSION >= 211000
  mstring = this->readMetadata();
#else
  mstring.reset(this->readMetadata());
#endif
  if (mstring.get() == nullptr)
    return "";
  const char *cstring = pdf::get_c_string(mstring.get());
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


/* class pdf::Timestamp
 * ====================
 */

pdf::Timestamp::Timestamp()
: dummy(true),
  timestamp{},
  tz_sign(0),
  tz_hour(0),
  tz_minute(0)
{ }

pdf::Timestamp pdf::Timestamp::now()
{
  pdf::Timestamp result;
  result.dummy = false;
  time_t unix_now = time(nullptr);
  if (unix_now == static_cast<time_t>(-1))
    throw_posix_error("time()");
  struct tm *local_tm = localtime(&unix_now);
  if (local_tm == nullptr)
    throw_posix_error("localtime()");
  time_t unix_now_l = timegm(local_tm);
  if (unix_now_l == static_cast<time_t>(-1))
    throw_posix_error("timegm()");
  time_t tz_offset = unix_now_l - unix_now;
  if (tz_offset >= 0)
    result.tz_sign = '+';
  else
  {
    result.tz_sign = '-';
    tz_offset = -tz_offset;
  }
  tz_offset /= 60;
  result.tz_hour = tz_offset / 60;
  result.tz_minute = tz_offset % 60;
  result.timestamp = *local_tm;
  return result;
}

pdf::Timestamp::Timestamp(int year, int month, int day, int hour, int minute, int second, char tz_sign, int tz_hour, int tz_minute)
: dummy(false),
  tz_sign(tz_sign),
  tz_hour(tz_hour),
  tz_minute(tz_minute)
{
  this->timestamp.tm_isdst = -1;
  this->timestamp.tm_year = year - 1900;
  this->timestamp.tm_mon = month - 1;
  this->timestamp.tm_mday = day;
  this->timestamp.tm_hour = hour;
  this->timestamp.tm_min = minute;
  this->timestamp.tm_sec = second;
}

std::string pdf::Timestamp::format(char separator) const
{
  /* Format timestamp according to RFC 3339 date format,
   * e.g. "2007-10-27S13:19:59+02:00", where S is the separator.
   */
  if (this->dummy)
    return "";
  std::ostringstream stream;
  char buffer[17 + CHAR_BIT * sizeof this->timestamp.tm_year / 3];
  char format[] = "%Y-%m-%d %H:%M:%S";
  format[8] = separator;
  struct tm tmp_timestamp = this->timestamp;
  if (mktime(&tmp_timestamp) == static_cast<time_t>(-1))
    throw pdf::Timestamp::Invalid();
  if (strftime(buffer, sizeof buffer, format, &this->timestamp) != 19)
    throw pdf::Timestamp::Invalid();
  stream << buffer;
  if (this->tz_sign)
  {
    if (this->tz_hour < 0 || this->tz_hour >= 24)
      throw pdf::Timestamp::Invalid();
    if (this->tz_minute < 0 || this->tz_minute >= 60)
      throw pdf::Timestamp::Invalid();
    stream
      << this->tz_sign
      << std::setw(2) << std::setfill('0') << this->tz_hour
      << ":"
      << std::setw(2) << std::setfill('0') << this->tz_minute
    ;
  }
  return stream.str();
}


/* class pdf::Metadata
 * ===================
 */

static int scan_date_digits(const char * &input, int n)
{
  int value = 0;
  for (int i = 0; i < n; i++)
  {
    if (*input >= '0' && *input <= '9' && value < INT_MAX / 10)
    {
      value *= 10;
      value += *input - '0';
    }
    else
      return INT_MIN;
    input++;
  }
  return value;
}

pdf::Metadata::Metadata(pdf::Document &document)
{
  string_fields.push_back({"Title", &this->title});
  string_fields.push_back({"Subject", &this->subject});
  string_fields.push_back({"Keywords", &this->keywords});
  string_fields.push_back({"Author", &this->author});
  string_fields.push_back({"Creator", &this->creator});
  string_fields.push_back({"Producer", &this->producer});
  date_fields.push_back({"CreationDate", &this->creation_date});
  date_fields.push_back({"ModDate", &this->mod_date});

  pdf::Object info;
  document.get_doc_info(info);
  if (!info.isDict())
    return;
  pdf::Dict *info_dict = info.getDict();
  for (auto &field : this->string_fields)
  {
    pdf::Object object;
    if (!pdf::dict_lookup(info_dict, field.first, &object)->isString())
      continue;
    *(field.second) = pdf::string_as_utf8(object);
  }
  for (auto &field : this->date_fields)
  {
    pdf::Object object;
    char tzs = 0; int tzh = 0, tzm = 0;
    if (!pdf::dict_lookup(info_dict, field.first, &object)->isString())
      continue;
    const char *input = pdf::get_c_string(object.getString());
    if (input[0] == 'D' && input[1] == ':')
      input += 2;
    int year = scan_date_digits(input, 4);
    int month = *input ? scan_date_digits(input, 2) : 1;
    int day = *input ? scan_date_digits(input, 2) : 1;
    int hour = *input ? scan_date_digits(input, 2) : 0;
    int minute = *input ? scan_date_digits(input, 2) : 0;
    int second  = *input ? scan_date_digits(input, 2) : 0;
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
      if (*input == '\'')
        input++;
      else
        tzh = -1;
      tzm = scan_date_digits(input, 2);
      if (*input == '\'')
        input++;
      else
        tzh = -1;
      break;
    case 'Z':
      input++;
      tzs = '+';
      tzh = tzm = 0;
      break;
    default:
      tzh = -1;
    }
    if (*input)
      tzh = -1;
    *(field.second) = Timestamp(year, month, day, hour, minute, second, tzs, tzh, tzm);
  }
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

bool pdf::Environment::antialias = false;

pdf::Renderer::Renderer(pdf::splash::Color &paper_color, bool monochrome)
: pdf::splash::OutputDevice(monochrome ? splashModeMono1 : splashModeRGB8, 4, false, paper_color),
  catalog(NULL)
{
  this->setFontAntialias(pdf::Environment::antialias);
  this->setVectorAntialias(pdf::Environment::antialias);
}

void pdf::Renderer::drawLink(pdf::link::Link *link, pdf::Catalog *catalog)
{
  std::string border_color;
  if (this->link_border_colors.size())
  {
    border_color = this->link_border_colors.back();
    this->link_border_colors.pop_back();
  }
  this->draw_link(link, border_color);
}


/* glyph-related functions
 * =======================
 */

bool pdf::get_glyph(splash::Splash *splash, splash::Font *font,
  double x, double y, int code, splash::GlyphBitmap *bitmap)
{
  if (font == nullptr)
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
  /* Source was copied from <poppler/SplashOutputDev.c>. */
  // for POPPLER_VERSION >= 8300:
  //   const pdf::gfx::Path *path
  auto path = state->getPath();
  int n_subpaths = path->getNumSubpaths();
  for (int i = 0; i < n_subpaths; i++)
  {
    // for POPPLER_VERSION >= 8300:
    //   const pdf::gfx::Subpath *subpath
    auto subpath = path->getSubpath(i);
    if (subpath->getNumPoints() > 0)
    {
      double x1, y1, x2, y2, x3, y3;
      state->transform(subpath->getX(0), subpath->getY(0), &x1, &y1);
      splash_path.moveTo(static_cast<splash::Coord>(x1), static_cast<splash::Coord>(y1));
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
            static_cast<splash::Coord>(x1), static_cast<splash::Coord>(y1),
            static_cast<splash::Coord>(x2), static_cast<splash::Coord>(y2),
            static_cast<splash::Coord>(x3), static_cast<splash::Coord>(y3)
          );
          j += 3;
        }
        else
        {
          state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
          splash_path.lineTo(static_cast<splash::Coord>(x1), static_cast<splash::Coord>(y1));
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
  unsigned char ch;
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
  *object = dict.dictLookup(const_cast<char*>(key));
  return object;
}

pdf::Object *pdf::dict_lookup(pdf::Object *dict, const char *key, pdf::Object *object)
{
  return pdf::dict_lookup(*dict, key, object);
}

pdf::Object *pdf::dict_lookup(pdf::Dict *dict, const char *key, pdf::Object *object)
{
  *object = dict->lookup(const_cast<char*>(key));
  return object;
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
          stream.put(static_cast<char>(row_ptr[x] ^ 0xFF));
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

#if POPPLER_VERSION >= 7200
const char * pdf::get_c_string(const pdf::String *str)
{
  return str->c_str();
}
#else
const char * pdf::get_c_string(const pdf::String *str)
{
  return str->getCString();
}
#endif

int pdf::find_page(pdf::Catalog *catalog, pdf::Ref pgref)
{
#if POPPLER_VERSION >= 7600
  return catalog->findPage(pgref);
#else
  return catalog->findPage(pgref.num, pgref.gen);
#endif
}

// vim:ts=2 sts=2 sw=2 et
