/* Copyright © 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "quantizer.hh"

#include "config.hh"
#include "version.hh"

#ifdef HAVE_GRAPHICSMAGICK
#include <Magick++.h>
#endif

void WebSafeQuantizer::output_web_palette(std::ostream &stream)
{
  stream << "216" << std::endl;
  for (int r = 0; r < 6; r++)
  for (int g = 0; g < 6; g++)
  for (int b = 0; b < 6; b++)
  {
    char buffer[] = { 51 * r, 51 * g, 51 * b };
    stream.write(buffer, 3);
  }
}

void WebSafeQuantizer::operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
  int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream)
{
  stream << "R6 " << width << " " << height << " ";
  output_web_palette(stream);
  pdf::Pixmap bmp_fg(out_fg);
  pdf::Pixmap bmp_bg(out_bg);
  pdf::Pixmap::iterator p_fg = bmp_fg.begin();
  pdf::Pixmap::iterator p_bg = bmp_bg.begin();
  for (int i = 0; i < 3; i++) 
    background_color[i] = p_bg[i];
  for (int y = 0; y < height; y++)
  {
    int new_color, color = 0xfff;
    int length = 0;
    for (int x = 0; x < width; x++)
    {
      if (!has_background)
      {
        for (int i = 0; i < 3; i++)
        if (background_color[i] != p_bg[i])
        {
          has_background = true;
          break;
        }
      }
      if (p_fg[0] != p_bg[0] || p_fg[1] != p_bg[1] || p_fg[2] != p_bg[2])
      {
        if (!has_foreground && (p_fg[0] || p_fg[1] || p_fg[2]))
          has_foreground = true;
        new_color = ((p_fg[2] + 1) / 43) + 6 * (((p_fg[1] + 1) / 43) + 6 * ((p_fg[0] + 1) / 43));
      }
      else
        new_color = 0xfff;
      if (color == new_color)
        length++;
      else
      {
        if (length > 0)
        {
          int item = (color << 20) + length;
          for (int i = 0; i < 4; i++)
          {
            char c = item >> ((3 - i) * 8);
            stream.write(&c, 1);
          }
        }
        color = new_color;
        length = 1;
      }
      p_fg++, p_bg++;
    }
    p_fg.next_row(), p_bg.next_row();
    int item = (color << 20) + length;
    for (int i = 0; i < 4; i++)
    {
      char c = item >> ((3 - i) * 8);
      stream.write(&c, 1);
    }
  }
}

void DummyQuantizer::operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
  int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream)
{
  static const int MAX_RUN_LENGTH = 0x3fff;
  stream << "R4 " << width << " " << height << " ";
  int width_remainder = width % MAX_RUN_LENGTH;
  if (width_remainder == 0)
    width_remainder = MAX_RUN_LENGTH;
  char run[2];
  int run_length = 1;
  std::string line;
  if (width_remainder < 192)
    run[0] = static_cast<char>(width_remainder);
  else
  {
    run[0] = static_cast<char>(0xc0 + (width_remainder >> 8));
    run[1] = static_cast<char>(width_remainder & 0xff);
    run_length++;
  }
  for (int y = 0; y < height; y++)
  {
    int remaining_width = width;
    while (remaining_width > 0x3fff)
    {
      stream.write("\xff\xff", 0);
      remaining_width -= 0x3fff;
    }
    stream.write(run, run_length);
  }
  background_color[0] = background_color[1] = background_color[2] = 0xff;
}

#ifdef HAVE_GRAPHICSMAGICK

GraphicsMagickQuantizer::GraphicsMagickQuantizer()
{ }

void GraphicsMagickQuantizer::operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
  int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream)
{
  stream << "R6 " << width << " " << height << " ";
  Magick::Image image(Magick::Geometry(width, height), Magick::Color());
  image.type(Magick::TrueColorMatteType);
  image.modifyImage();
  pdf::Pixmap bmp_fg(out_fg);
  pdf::Pixmap bmp_bg(out_bg);
  pdf::Pixmap::iterator p_fg = bmp_fg.begin();
  pdf::Pixmap::iterator p_bg = bmp_bg.begin();
  for (int i = 0; i < 3; i++) 
    background_color[i] = p_bg[i];
  for (int y = 0; y < height; y++)
  {
    Magick::PixelPacket* ipixel = image.setPixels(0, y, width, 1);
    for (int x = 0; x < width; x++)
    {
      if (!has_background)
      {
        for (int i = 0; i < 3; i++)
        if (background_color[i] != p_bg[i])
        {
          has_background = true;
          break;
        }
      }
      if (p_fg[0] != p_bg[0] || p_fg[1] != p_bg[1] || p_fg[2] != p_bg[2])
      {
        if (!has_foreground && (p_fg[0] || p_fg[1] || p_fg[2]))
          has_foreground = true;
        *ipixel = Magick::Color(p_fg[0], p_fg[1], p_fg[2], 0);
      }
      else
        *ipixel = Magick::Color(0, 0, 0, 0xff);
      p_fg++, p_bg++, ipixel++;
    }
    p_fg.next_row(), p_bg.next_row(), image.syncPixels();
  }
  image.quantizeColorSpace(Magick::TransparentColorspace);
  image.quantizeColors(config::fg_colors);
  image.quantize();
  image.colorSpace(Magick::RGBColorspace);
  image.quantizeColorSpace(Magick::RGBColorspace);
  image.quantizeColors(9999);
  image.quantize();
  unsigned int n_colors = image.colorMapSize();
  stream << n_colors << std::endl;
  for (unsigned int i = 0; i < n_colors; i++)
  {
    const Magick::Color &color = image.colorMap(i);
    char buffer[] = { color.redQuantum(), color.greenQuantum(), color.blueQuantum() };
    stream.write(buffer, 3);
  }
  for (int y = 0; y < height; y++)
  {
    int new_color, color = 0xfff;
    Magick::PixelPacket *ipixel = image.getPixels(0, y, width, 1);
    Magick::IndexPacket *ppixel = image.getIndexes();
    int length = 0;
    for (int x = 0; x < width; x++)
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
        {
          int item = (color << 20) + length;
          for (int i = 0; i < 4; i++)
          {
            char c = item >> ((3 - i) * 8);
            stream.write(&c, 1);
          }
        }
        color = new_color;
        length = 1;
      }
      ipixel++, ppixel++;
    }
    int item = (color << 20) + length;
    for (int i = 0; i < 4; i++)
    {
      char c = item >> ((3 - i) * 8);
      stream.write(&c, 1);
    }
  }
}

#else

GraphicsMagickQuantizer::GraphicsMagickQuantizer()
{ 
  throw NotImplementedError();
}

void GraphicsMagickQuantizer::operator()(pdf::Renderer *out_fg, pdf::Renderer *out_bg, int width, int height,
  int *background_color, bool &has_foreground, bool &has_background, std::ostream &stream)
{ /* just to satisfy compilers */ }

#endif

// vim:ts=2 sw=2 et
