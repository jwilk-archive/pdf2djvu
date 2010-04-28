/* Copyright © 2008, 2009, 2010 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "version.hh"

#if HAVE_GRAPHICSMAGICK
#include <Magick++.h>
#endif

#include <bitset>
#include <map>
#include <vector>

#include "image-filter.hh"

#include "config.hh"
#include "djvuconst.hh"
#include "rle.hh"

void image::Bitmap::output_r4(std::ostream &stream)
{
  stream << "R4 " << this->width << " " << this->height << "\n";
  for (unsigned int y = 0; y < this->height; y++)
  {
    const std::vector<uint8_t> run = this->runs[y];
    std::vector<uint8_t>::const_iterator it;
    for (it = run.begin(); it != run.end(); it++)
    {
      uint8_t length = *it;
      if (length >= 192)
        stream
          << static_cast<char>(0xc0 + (length >> 8))
          << static_cast<char>(length & 0xff);
      else
        stream << static_cast<char>(length);
    }
  }
}

void image::Filter::filter(FilterData &filter_data)
{
  this->black_foreground = true;
  this->solid_background = true;
  this->width = filter_data.width;
  this->height = filter_data.height;
  this->sep_stream = &filter_data.sep_stream;
  this->background_color[0] = this->background_color[1] = this->background_color[2] = 0xff;
  { /* Extract image (foreground + background). */
    this->image.reset();
    this->extract_image(filter_data.r1);
  }
  { /* Extract mask. */
    bool ignorable = &filter_data.r1 == &filter_data.r2;
    this->mask.reset();
    this->extract_mask(filter_data.r2, ignorable);
  }
  this->process_image();
}

void image::Filter::extract_image(pdf::Renderer &r)
{
  this->image.reset(new pdf::Pixmap(&r));
}

void image::Filter::extract_mask(pdf::Renderer &r2, bool ignorable)
{
  this->black_foreground = true;
  this->solid_background = true;
  pdf::Pixmap::iterator it1 = this->image->begin();
  if (ignorable)
  { /* Optimize the case when mask is a priori known to be empty. */
    for (unsigned int y = 0; y < height; y++)
    {
      for (unsigned int x = 0; x < width; x++)
      {
        if (this->solid_background)
        {
          for (unsigned int i = 0; i < 3; i++)
          if (background_color[i] != it1[i])
          {
            this->solid_background = false;
            return;
          }
        }
        it1++;
      }
      it1.next_row();
    }
  }
  pdf::Pixmap image2(&r2);
  pdf::Pixmap::iterator it2 = image2.begin();
  this->mask.reset(new image::Bitmap(this->width, this->height));
  for (unsigned int y = 0; y < height; y++)
  {
    for (unsigned int x = 0; x < width; x++)
    {
      if (this->solid_background)
      {
        for (unsigned int i = 0; i < 3; i++)
        if (background_color[i] != it2[i])
        {
          this->solid_background = false;
          break;
        }
      }
      if (it1[0] != it2[0] || it1[1] != it2[1] || it1[2] != it2[2])
      {
        if (!this->black_foreground && (it1[0] || it1[1] || it1[2]))
          this->black_foreground = true;
        *this->mask << 1;
      }
      else
        *this->mask << 0;
      it1++, it2++;
    }
    it1.next_row(), it2.next_row();
    this->mask->next_row();
  }
}

void image::MaskFilter::process_image()
{
  this->mask->output_r4(*this->sep_stream);
}

void image::WebSafeFilter::process_image()
{
  rle::R6 r6(*this->sep_stream, width, height);
  r6.use_web_palette();

  pdf::Pixmap::iterator i_image = this->image->begin();
  image::Bitmap::iterator i_mask = this->mask->begin();
  for (unsigned int i = 0; i < 3; i++)
    background_color[i] = i_image[i];
  for (unsigned int y = 0; y < this->height; y++)
  {
    unsigned int new_color, color = 0xfff;
    unsigned int length = 0;
    for (unsigned int x = 0; x < this->width; x++)
    {
      new_color =
        *i_mask
        ? ((i_image[2] + 1) / 43) + 6 * (((i_image[1] + 1) / 43) + 6 * ((i_image[0] + 1) / 43))
        : 0xfff;
      if (color == new_color)
        length++;
      else
      {
        if (length > 0)
          r6.output_run(color, length);
        color = new_color;
        length = 1;
      }
      i_image++, i_mask++;
    }
    i_image.next_row(), i_image.next_row();
    r6.output_run(color, length);
  }
}

class Rgb18
{
protected:
  int value;
public:
  explicit Rgb18()
  : value(-1)
  { }

  explicit Rgb18(int r, int g, int b)
  : value((r >> 2) | ((g >> 2) << 6) | ((b >> 2) << 12))
  { }

  explicit Rgb18(size_t n)
  : value(n)
  { }

  template <typename tp>
  explicit Rgb18(const tp &components)
  : value((components[0] >> 2) | ((components[1] >> 2) << 6) | ((components[2] >> 2) << 12))
  { }

  int operator [](int i) const
  {
    return
      (((this->value >> (6 * i)) << 2) & 0xff) |
      ((this->value >> (6 * i + 4)) & 3);
  }

  bool operator ==(Rgb18 other) const
  {
    return this->value == other.value;
  }

  operator int () const
  {
    return this->value;
  }

  Rgb18 reduce(int k) const
  {
    int components[3];
    const int n = 1 << 8;
    const int c = (n + k - 1) / k;
    for (int i = 0; i < 3; i++)
    {
      const int m = ((*this)[i] * c) / n;
      components[i] = (n - 1) * m / (c - 1);
    }
    return Rgb18(components);
  }

};

class Run
{
protected:
  Rgb18 color;
  unsigned int length;
public:
  explicit Run(Rgb18 color)
  : color(color), length(0)
  { }
  explicit Run()
  : color(Rgb18()), length(0)
  { }
  void operator ++(int)
  {
    this->length++;
  }
  bool same_color(Rgb18 other_color) const
  {
    return this->color == other_color;
  }
  Rgb18 get_color() const
  {
    return this->color;
  }
  unsigned int get_length() const
  {
    return this->length;
  }
};

void image::DefaultFilter::process_image()
{
  rle::R6 r6(*this->sep_stream, width, height);
  pdf::Pixmap::iterator i_image = this->image->begin();
  image::Bitmap::iterator i_mask = this->mask->begin();
  unsigned int color_counter = 0;
  std::bitset<1 << 18> original_colors;
  std::bitset<1 << 18> quantized_colors;
  std::vector<std::vector<Run> > runs(height);
  for (unsigned int y = 0; y < height; y++)
  {
    Run run;
    Rgb18 new_color;
    for (unsigned int x = 0; x < width; x++)
    {
      if (*i_mask)
      {
        new_color = Rgb18(i_image[0], i_image[1], i_image[2]);
        if (!original_colors[new_color])
        {
          color_counter++;
          original_colors.set(new_color);
        }
      }
      else
        new_color = Rgb18();
      if (run.same_color(new_color))
        run++;
      else
      {
        if (run.get_length() > 0)
          runs[y].push_back(run);
        run = Run(new_color);
        run++;
      }
      i_image++, i_mask++;
    }
    i_image.next_row(), i_mask.next_row();
    if (run.get_length() > 0)
      runs[y].push_back(run);
  }
  /* Find appropriate color palette: */
  int divisor = 4;
  while (color_counter > djvu::max_fg_colors)
  {
    unsigned int new_color_counter = 0;
    quantized_colors.reset();
    divisor++;
    for (size_t color = 0; color < original_colors.size(); color++)
    {
      if (!original_colors[color])
        continue;
      Rgb18 new_color = Rgb18(color).reduce(divisor);
      if (!quantized_colors[new_color])
      {
        quantized_colors.set(new_color);
        new_color_counter++;
        if (new_color_counter > djvu::max_fg_colors)
          break;
      }
    }
    color_counter = new_color_counter;
  }
  if (divisor == 4)
    quantized_colors = original_colors;
  /* Output the palette: */
  if (color_counter == 0)
    r6.use_dummy_palette();
  else
  {
    r6.use_palette(color_counter);
    for (size_t color = 0; color < quantized_colors.size(); color++)
      if (quantized_colors[color])
      {
        Rgb18 rgb18(color);
        r6.add_color(rgb18[0], rgb18[1], rgb18[2]);
      }
  }
  /* Map colors into color indices: */
  std::map<int, unsigned int> color_map;
  int last_color_index = 0;
  color_map[-1] = 0xfff;
  if (divisor == 4)
    for (size_t color = 0; color < original_colors.size(); color++)
    {
      if (!original_colors[color])
        continue;
      color_map[color] = last_color_index++;
    }
  else
  {
    std::map<int, unsigned int> quantized_color_map;
    for (size_t color = 0; color < quantized_colors.size(); color++)
    {
      if (!quantized_colors[color])
        continue;
      quantized_color_map[color] = last_color_index++;
    }
    for (size_t color = 0; color < original_colors.size(); color++)
    {
      Rgb18 new_color = Rgb18(color).reduce(divisor);
      color_map[color] = quantized_color_map[new_color];
    }
  }
  /* Output runs: */
  for (unsigned int y = 0; y < height; y++)
  {
    const std::vector<Run> line_runs = runs[y];
    for (std::vector<Run>::const_iterator run = line_runs.begin(); run != line_runs.end(); run++)
    {
      unsigned int color_index = color_map[run->get_color()];
      r6.output_run(color_index, run->get_length());
    }
  }
}

void image::DummyFilter::extract_image(pdf::Renderer &)
{
  this->image.reset(NULL);
}

void image::DummyFilter::extract_mask(pdf::Renderer &, bool ignorable)
{
  this->mask.reset(NULL);
}

void image::DummyFilter::process_image()
{
  rle::R4 r4(*this->sep_stream, width, height);
  for (unsigned int y = 0; y < height; y++)
    r4.output_run(width);
  this->background_color[0] = this->background_color[1] = this->background_color[2] = 0xff;
}

#if HAVE_GRAPHICSMAGICK

void image::GraphicsMagickFilter::process_image()
{
  rle::R6 r6(*this->sep_stream, width, height);
  Magick::Image image(Magick::Geometry(width, height), Magick::Color());
  image.type(Magick::TrueColorMatteType);
  image.modifyImage();
  pdf::Pixmap::iterator i_image = this->image->begin();
  image::Bitmap::iterator i_mask = this->mask->begin();
  for (int i = 0; i < 3; i++)
    background_color[i] = i_image[i];
  for (unsigned int y = 0; y < height; y++)
  {
    Magick::PixelPacket* ipixel = image.setPixels(0, y, width, 1);
    for (unsigned int x = 0; x < width; x++)
    {
      *ipixel =
        *i_mask
        ? Magick::Color(i_image[0], i_image[1], i_image[2], 0)
        : Magick::Color(0, 0, 0, 0xff);
      i_image++, i_mask++;
    }
    i_image++, i_mask++, image.syncPixels();
  }
  image.quantizeColorSpace(Magick::TransparentColorspace);
  image.quantizeColors(this->config.fg_colors);
  image.quantize();
  image.colorSpace(Magick::RGBColorspace);
  image.quantizeColorSpace(Magick::RGBColorspace);
  image.quantizeColors(9999);
  image.quantize();
  unsigned int n_colors = image.colorMapSize();
  r6.use_palette(n_colors);
  for (unsigned int i = 0; i < n_colors; i++)
  {
    const Magick::Color &color = image.colorMap(i);
    r6.add_color(color.redQuantum(), color.greenQuantum(), color.blueQuantum());
  }
  for (unsigned int y = 0; y < height; y++)
  {
    int new_color, color = 0xfff;
    Magick::PixelPacket *ipixel = image.getPixels(0, y, width, 1);
    Magick::IndexPacket *ppixel = image.getIndexes();
    int length = 0;
    for (unsigned int x = 0; x < width; x++)
    {
      if (ipixel->opacity != TransparentOpacity)
        new_color = *ppixel;
      else
        new_color = 0xfff;
      if (color == new_color)
        length++;
      else
      {
        if (length > 0)
          r6.output_run(color, length);
        color = new_color;
        length = 1;
      }
      ipixel++, ppixel++;
    }
    r6.output_run(color, length);
  }
}

#else

image::GraphicsMagickFilter::process_image()
{
  throw NotImplementedError();
}

#endif

// vim:ts=2 sw=2 et
