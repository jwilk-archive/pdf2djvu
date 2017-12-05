/* Copyright © 2008-2017 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_IMAGE_FILTER_H
#define PDF2DJVU_IMAGE_FILTER_H

#include <ostream>
#include <stdexcept>

#include "pdf-backend.hh"
#include "config.hh"
#include "i18n.hh"

class Quantizer
{
protected:
  const Config &config;
public:
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream) = 0;
  explicit Quantizer(const Config &config) : config(config) { }
  virtual ~Quantizer()
  { }
};

class DefaultQuantizer : public Quantizer
{
public:
  explicit DefaultQuantizer(const Config &config)
  : Quantizer(config)
  { }
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class WebSafeQuantizer : public Quantizer
{
protected:
  void output_web_palette(std::ostream &stream);
public:
  explicit WebSafeQuantizer(const Config &config)
  : Quantizer(config)
  { }
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class MaskQuantizer : public Quantizer
{
public:
  explicit MaskQuantizer(const Config &config)
  : Quantizer(config)
  { }
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class DummyQuantizer : public Quantizer
{
public:
  explicit DummyQuantizer(const Config &config)
  : Quantizer(config)
  { }
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class GraphicsMagickQuantizer : public Quantizer
{
public:
  explicit GraphicsMagickQuantizer(const Config &config);
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
  class NotImplementedError : public std::runtime_error
  {
  public:
    NotImplementedError()
    : std::runtime_error(_("pdf2djvu was built without GraphicsMagick; advanced color quantization is disabled."))
    { }
  };
};

#endif

// vim:ts=2 sts=2 sw=2 et
