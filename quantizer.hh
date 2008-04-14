/* Copyright © 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_QUANTIZER_H
#define PDF2DJVU_QUANTIZER_H

#include "compoppler.hh"

class Quantizer
{
public:
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream) = 0;
  virtual ~Quantizer() { /* just to shut up compilers */ }
};

class WebSafeQuantizer : public Quantizer
{
protected:
  void output_web_palette(std::ostream &stream);
public:
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class DummyQuantizer : public WebSafeQuantizer
{
public:
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

class GraphicsMagickQuantizer : public Quantizer
{
public:
  GraphicsMagickQuantizer();
  virtual void operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
    int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream);
};

#endif

// vim:ts=2 sw=2 et
