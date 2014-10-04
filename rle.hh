/* Copyright © 2010 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_RLE_H
#define PDF2DJVU_RLE_H

#include <cassert>
#include <ostream>

/* Support for RLE formats.
 * Please refer to csepdjvu(1) for the format specifications.
 */

namespace rle
{
  class R4
  {
  protected:
    std::ostream &stream;
    unsigned int x, width, height;
    unsigned int run_length;
    int last_pixel;
  public:
    template <typename T> R4(std::ostream &, T width, T height);
    void operator <<(int pixel);
    template <typename T> void output_run(T);
  };
}

template <typename T>
rle::R4::R4(std::ostream &stream, T width_, T height_)
: stream(stream),
  x(0), width(width_), height(height_),
  run_length(0),
  last_pixel(0)
{
  assert(width_ > 0);
  assert(height_ > 0);
  assert(static_cast<T>(this->width) == width_);
  assert(static_cast<T>(this->height) == height_);
  this->stream << "R4 " << this->width << " " << this->height << " ";
}

void rle::R4::operator <<(int pixel)
{
  pixel = !!pixel;
  this->x++;
  assert(this->x > 0);
  if (this->last_pixel != pixel)
  {
    this->output_run(this->run_length);
    this->run_length = 1;
    this->last_pixel = pixel;
  }
  else
    this->run_length++;
  if (this->x == this->width)
  {
    this->output_run(this->run_length);
    this->last_pixel = 0;
    this->x = 0;
    this->run_length = 0;
  }
}

template <typename T>
void rle::R4::output_run(T length_)
{
  unsigned int length = length_;
  static const unsigned int max_length = 0x3fff;
  assert(length_ >= 0);
  assert(static_cast<T>(length) == length_);
  assert(length <= this->width);
  while (length > max_length)
  {
    this->stream.write("\xff\xff", 3);
    length -= max_length;
  }
  if (length >= 192)
  {
    this->stream
      << static_cast<char>(0xc0 + (length >> 8))
      << static_cast<char>(length & 0xff);
  }
  else
    this->stream << static_cast<char>(length);
}

#endif

// vim:ts=2 sts=2 sw=2 et
