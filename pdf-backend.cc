/* Copyright © 2007, 2008, 2009, 2010, 2012 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "debug.hh"
#include "i18n.hh"
#include "pdf-backend.hh"
#include "system.hh"

#include <Error.h>
#include <GlobalParams.h>
#include <PDFDocEncoding.h>
#if (POPPLER_VERSION < 2100) || (POPPLER_VERSION >= 2101)
#include <UTF8.h>
#endif
#if POPPLER_VERSION >= 2100
#include <UTF.h>
#endif
#include <UnicodeTypeTable.h>

/* class pdf::Environment
 * ======================
 */

#if POPPLER_VERSION < 1900
static void poppler_error_handler(pdf::Offset pos, char *message, va_list args)
{
  std::string format;
  std::string expanded_message = string_vprintf(message, args);
  const char *c_message = expanded_message.c_str();
  const char *category = _("PDF error");
  if (pos >= 0)
  {
    error_log <<
      /* L10N: "<error-category> (<position>): <error-message>" */
      string_printf(_("%s (%jd): %s"), category, static_cast<intmax_t>(pos), c_message);
  }
  else
  {
    error_log <<
      /* L10N: "<error-category>: <error-message>" */
      string_printf(_("%s: %s"), category, c_message);
  }
  error_log << std::endl;
}
#else
static void poppler_error_handler(void *data, ErrorCategory category, pdf::Offset pos, char *message)
{
  std::string format;
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
#endif

pdf::Environment::Environment(const char *argv0)
{
#if WIN32
  /* Change the current working directory to be able to read poppler data.
   * This is not required (and potentially harmful) for Unix installations.
   */
  std::string argv0_dir_name, argv0_file_name;
  split_path(argv0, argv0_dir_name, argv0_file_name);
  Cwd cwd(argv0_dir_name);
#endif
  globalParams = new GlobalParams();
#if POPPLER_VERSION < 1900
  setErrorFunction(poppler_error_handler);
#else
  setErrorCallback(poppler_error_handler, NULL);
#endif
}

void pdf::Environment::set_antialias(bool value)
{
  if (!globalParams->setAntialias(const_cast<char*>(value ? "yes" : "no")))
    throw UnableToSetParameter(_("Unable to set antialias parameter"));
  if (!globalParams->setVectorAntialias(const_cast<char*>(value ? "yes" : "no")))
    throw UnableToSetParameter(_("Unable to set vector antialias parameter"));
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

static pdf::Bool annotations_callback(pdf::ant::Annotation *annotation, void *user_data)
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


/* class pdf::Timestamp
 * ====================
 */

pdf::Timestamp::Timestamp()
: dummy(true)
{ }

pdf::Timestamp pdf::Timestamp::now()
{
  time_t unix_now;
  pdf::Timestamp result;
  result.dummy = false;
  result.tz_sign = '+';
  result.tz_hour = 0;
  result.tz_minute = 0;
  time(&unix_now);
  if (unix_now == static_cast<time_t>(-1))
    throw pdf::Timestamp::Invalid();
  struct tm *c_timestamp = gmtime(&unix_now);
  if (c_timestamp == NULL)
    throw pdf::Timestamp::Invalid();
  result.timestamp = *c_timestamp;
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
    throw Invalid();
  if (strftime(buffer, sizeof buffer, format, &this->timestamp) != 19)
    throw Invalid();
  stream << buffer;
  if (this->tz_sign)
  {
    if (this->tz_hour < 0 || this->tz_hour >= 24)
      throw Invalid();
    if (this->tz_minute < 0 || this->tz_minute >= 60)
      throw Invalid();
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

static int scan_date_digits(char * &input, int n)
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
  string_fields.push_back(std::make_pair("Title", &this->title));
  string_fields.push_back(std::make_pair("Subject", &this->subject));
  string_fields.push_back(std::make_pair("Keywords", &this->keywords));
  string_fields.push_back(std::make_pair("Author", &this->author));
  string_fields.push_back(std::make_pair("Creator", &this->creator));
  string_fields.push_back(std::make_pair("Producer", &this->producer));
  date_fields.push_back(std::make_pair("CreationDate", &this->creation_date));
  date_fields.push_back(std::make_pair("ModDate", &this->mod_date));

  pdf::OwnedObject info;
  document.getDocInfo(&info);
  if (!info.isDict())
    return;
  pdf::Dict *info_dict = info.getDict();
  for (std::vector<string_field>::iterator it = this->string_fields.begin(); it != this->string_fields.end(); it++)
  {
    pdf::OwnedObject object;
    if (!pdf::dict_lookup(info_dict, it->first, &object)->isString())
      continue;
    *it->second = pdf::string_as_utf8(object);
  }
  for (std::vector<date_field>::iterator it = this->date_fields.begin(); it != this->date_fields.end(); it++)
  {
    pdf::OwnedObject object;
    char tzs = 0; int tzh = 0, tzm = 0;
    if (!pdf::dict_lookup(info_dict, it->first, &object)->isString())
      continue;
    char *input = object.getString()->getCString();
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
    *it->second = Timestamp(year, month, day, hour, minute, second, tzs, tzh, tzm);
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
  /* Source was copied from <poppler/SplashOutputDev.c>. */
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
        code = ((cstring[i] & 0xff) << 8) + (cstring[i + 1] & 0xff);
      else
      {
        /* A lone byte is encountered. */
        code = replacement_character;
      }
      if (code_shift)
      {
        /* A leading surrogate has been encountered. */
        if (code >= 0xdc00 && code < 0xe000)
        {
          /* A trailing surrogate is encountered. */
          code = code_shift + (code & 0x3ff);
          if (code >= 0x110000)
            code = replacement_character;
        }
        else
        {
          /* An unpaired surrogate is encountered. */
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
          if (code < (1U << (5 * nbytes + 1)))
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
: data(NULL), int_length(0)
{
  data = unicodeNormalizeNFKC(unistr, length, &this->int_length, NULL);
}

pdf::NFKC::~NFKC() throw ()
{
  gfree(this->data);
}

#if POPPLER_VERSION >= 1900
/* Preempt this poppler function, so that it doesn't stand in our way. */
pdf::Bool unicodeIsAlphabeticPresentationForm(Unicode c) {
  return 0;
}
#endif

// vim:ts=2 sw=2 et
