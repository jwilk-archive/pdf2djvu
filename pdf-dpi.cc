/* Copyright © 2009-2019 Jakub Wilk <jwilk@jwilk.net>
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

#include "pdf-dpi.hh"

#include <algorithm>
#include <cmath>
#include <limits>

#include "pdf-backend.hh"

class DpiGuessDevice : public pdf::OutputDevice
{
protected:
  double min_;
  double max_;
  void process_image(pdf::gfx::State *state, int width, int height);

  virtual void drawImageMask(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    bool invert, bool interpolate, bool inline_image)
  {
    this->process_image(state, width, height);
  }

#if POPPLER_VERSION >= 8200
  virtual void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, bool interpolate, const int *mask_colors, bool inline_image)
#else
  virtual void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, bool interpolate, int *mask_colors, bool inline_image)
#endif
  {
    this->process_image(state, width, height);
  }

  virtual void drawMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height, bool mask_invert, bool mask_interpolate)
  {
    this->process_image(state, width, height);
    this->process_image(state, mask_width, mask_height);
  }

  virtual void drawSoftMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream,
    int width, int height, pdf::gfx::ImageColorMap *color_map, bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height,
    pdf::gfx::ImageColorMap *mask_color_map, bool mask_interpolate)
  {
    this->process_image(state, width, height);
    this->process_image(state, mask_width, mask_height);
  }

  virtual bool interpretType3Chars()
  {
    return false;
  }
  virtual bool upsideDown()
  {
    return false;
  }
  virtual bool useDrawChar()
  {
    return false;
  }
public:
  DpiGuessDevice()
  {
    this->reset();
  }
  void reset()
  {
    this->max_ = 0.0;
    this->min_ = std::numeric_limits<double>::infinity();
  }
  double min() const { return this->min_; }
  double max() const { return this->max_; }
  virtual ~DpiGuessDevice()
  { }
};

void DpiGuessDevice::process_image(pdf::gfx::State *state, int width, int height)
{
  const double *ctm = state->getCTM();
  double h_dpi = 72.0 * width / hypot(ctm[0], ctm[1]);
  double v_dpi = 72.0 * height / hypot(ctm[2], ctm[3]);
  this->min_ = std::min(this->min_, std::min(h_dpi, v_dpi));
  this->max_ = std::max(this->max_, std::max(h_dpi, v_dpi));
}

pdf::dpi::Guesser::Guesser(pdf::Document &document)
: document(document)
{
  DpiGuessDevice *guess_device = new DpiGuessDevice();
  this->magic = guess_device;
}

pdf::dpi::Guesser::~Guesser()
{
  DpiGuessDevice *guess_device = static_cast<DpiGuessDevice*>(this->magic);
  delete guess_device;
}

pdf::dpi::Guess pdf::dpi::Guesser::operator[](int n)
{
  DpiGuessDevice *guess_device = static_cast<DpiGuessDevice*>(this->magic);
  guess_device->reset();
  this->document.displayPages(guess_device, n, n, 72, 72, 0, true, false, false);
  double min = guess_device->min();
  double max = guess_device->max();
  if (max == 0.0)
    throw pdf::dpi::NoGuess();
  return pdf::dpi::Guess(min, max);
}

// vim:ts=2 sts=2 sw=2 et
