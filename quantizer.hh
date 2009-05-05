/* Copyright © 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_QUANTIZER_H
#define PDF2DJVU_QUANTIZER_H

#include <stdexcept>

#include "pdf-backend.hh"
#include "config.hh"

class Quantizer
{
protected:
  const Config &config;
public:
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream) = 0;
  explicit Quantizer(const Config &config) : config(config) { }
  virtual ~Quantizer() throw ()
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

class DummyQuantizer : public WebSafeQuantizer
{
public:
  explicit DummyQuantizer(const Config &config)
  : WebSafeQuantizer(config)
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
    : std::runtime_error("Advanced color quantization is not supported.")
    { };
  };
};

#endif

// vim:ts=2 sw=2 et
