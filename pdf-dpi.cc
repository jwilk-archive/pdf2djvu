/* Copyright © 2009, 2011 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "pdf-dpi.hh"

#include <limits>

class DpiGuessDevice : public pdf::OutputDevice
{
protected:
  double min_;
  double max_;
  void process_image(pdf::gfx::State *state, int width, int height);

#if POPPLER_VERSION < 1101
  virtual void drawImageMask(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::Bool invert, pdf::Bool inline_image)
#else
  virtual void drawImageMask(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::Bool invert, pdf::Bool interpolate, pdf::Bool inline_image)
#endif
  {
    this->process_image(state, width, height);
  }
#if POPPLER_VERSION < 1101
  virtual void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, int *mask_colors, pdf::Bool inline_image)
#else
  virtual void drawImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate, int *mask_colors, pdf::Bool inline_image)
#endif
  {
    this->process_image(state, width, height);
  }

#if POPPLER_VERSION < 1101
  virtual void drawMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Stream *mask_stream, int mask_width, int mask_height, pdf::Bool mask_invert)
#else
  virtual void drawMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream, int width, int height,
    pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height, pdf::Bool mask_invert, pdf::Bool mask_interpolate)
#endif
  {
    this->process_image(state, width, height);
    this->process_image(state, mask_width, mask_height);
  }

#if POPPLER_VERSION < 1101
  virtual void drawSoftMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream,
    int width, int height, pdf::gfx::ImageColorMap *color_map, pdf::Stream *mask_stream,
    int mask_width, int mask_height, pdf::gfx::ImageColorMap *mask_color_map)
#else
  virtual void drawSoftMaskedImage(pdf::gfx::State *state, pdf::Object *object, pdf::Stream *stream,
    int width, int height, pdf::gfx::ImageColorMap *color_map, pdf::Bool interpolate,
    pdf::Stream *mask_stream, int mask_width, int mask_height,
    pdf::gfx::ImageColorMap *mask_color_map, pdf::Bool mask_interpolate)
#endif
  {
    this->process_image(state, width, height);
    this->process_image(state, mask_width, mask_height);
  }

  virtual GBool interpretType3Chars()
  {
    return false;
  }
  virtual GBool upsideDown()
  {
    return false;
  }
  virtual GBool useDrawChar()
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
  double *ctm = state->getCTM();
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

// vim:ts=2 sw=2 et
