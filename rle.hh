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

  class R6
  {
  protected:
    std::ostream &stream;
    bool has_palette;
    unsigned int n_colors;
    unsigned int width, height;
  public:
    template <typename T> R6(std::ostream &, T width, T height);
    template <typename T> void output_run(unsigned int color, T length);
    void use_palette(unsigned int n_colors);
    void add_color(uint8_t, uint8_t, uint8_t);
    void use_dummy_palette();
    void use_web_palette();
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

template <typename T>
rle::R6::R6(std::ostream &stream, T width_, T height_)
: stream(stream),
  has_palette(false), n_colors(0),
  width(width_), height(height_)
{
  assert(width_ > 0);
  assert(height_ > 0);
  assert(static_cast<T>(this->width) == width_);
  assert(static_cast<T>(this->height) == height_);
  this->stream << "R6 " << this->width << " " << this->height << " ";
}

void rle::R6::use_palette(unsigned int n_colors)
{
  assert(!this->has_palette);
  assert(n_colors > 0);
  assert(n_colors < 0xff0);
  this->n_colors = n_colors;
  this->stream << n_colors << " ";
}

void rle::R6::add_color(uint8_t r, uint8_t g, uint8_t b)
{
  assert(!this->has_palette);
  assert(this->n_colors-- > 0);
  this->stream
    << static_cast<char>(r)
    << static_cast<char>(g)
    << static_cast<char>(b);
  this->has_palette = this->n_colors == 0;
}

void rle::R6::use_dummy_palette()
{
  this->use_palette(1);
  this->add_color(0xff, 0xff, 0xff);
  assert(this->has_palette);
}

void rle::R6::use_web_palette()
{
  this->use_palette(216);
  for (unsigned int r = 0; r < 6; r++)
  for (unsigned int g = 0; g < 6; g++)
  for (unsigned int b = 0; b < 6; b++)
    this->add_color(51 * r, 51 * g, 51 * b);
  assert(this->has_palette);
}

static inline void write_uint32(std::ostream &stream, uint32_t item)
{
  unsigned char buffer[4];
  for (size_t i = 0; i < 4; i++)
    buffer[i] = item >> ((3 - i) * 8);
  stream.write(reinterpret_cast<char*>(buffer), 4);
}

template <typename T>
void rle::R6::output_run(unsigned int color, T length_)
{
  unsigned int length = length_;
  static const unsigned int max_length = 0xfffff;
  assert(this->has_palette);
  assert(length_ >= 0);
  assert(static_cast<T>(length) == length_);
  assert(length <= this->width);
  while (length > max_length)
  {
    write_uint32(this->stream, (((uint32_t) color) << 20) + (uint32_t) max_length);
    length -= max_length;
  }
  write_uint32(this->stream, (((uint32_t) color) << 20) + (uint32_t) length);
}

#endif

// vim:ts=2 sw=2 et
