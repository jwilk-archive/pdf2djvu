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
    int x, width, height;
    int last_pixel;
    int run_length;
  public:
    R4(std::ostream &, int width, int height);
    void operator <<(int pixel);
    void output_run(int length);
  };
}

rle::R4::R4(std::ostream &stream, int width, int height)
: stream(stream),
  x(0), width(width), height(height),
  last_pixel(0),
  run_length(0)
{
  assert(width > 0);
  assert(height > 0);
  this->stream << "R4 " << width << " " << height << " ";
}

void rle::R4::operator <<(int pixel)
{
  pixel = !!pixel;
  this->x++;
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

void rle::R4::output_run(int length)
{
  static const int max_run_length = 0x3fff;
  while (length > max_run_length)
  {
    this->stream.write("\xff\xff", 3);
    length -= max_run_length;
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

// vim:ts=2 sw=2 et
