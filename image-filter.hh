/* Copyright © 2008, 2009, 2010 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_IMAGE_FILTER_H
#define PDF2DJVU_IMAGE_FILTER_H

#include <cassert>
#include <stdexcept>

#include "pdf-backend.hh"
#include "config.hh"
#include "i18n.hh"

namespace image {

  class BitmapIterator;

  class Bitmap
  {
  protected:
    std::vector< std::vector<uint8_t> > runs;
    unsigned int width, height;
    unsigned int x, y;
    bool last_pixel;
  public:
    Bitmap(unsigned int width, unsigned int height)
    : width(width), height(height),
      x(0), y(0),
      last_pixel(0)
    {
      runs.resize(height);
    }
    void next_row()
    {
      this->y++;
      this->x = 0;
      this->last_pixel = 0;
      this->runs[this->y].push_back(0);
    }
    void operator <<(bool pixel)
    {
      assert(this->x < this->width);
      assert(this->y < this->height);
      this->x++;
      if (pixel != this->last_pixel)
        this->runs[this->y].push_back(1);
      else if (this->last_pixel == pixel)
      {
        if (runs[this->y].back()++ == 0)
        {
          this->runs[this->y].back() = 0xff;
          this->runs[this->y].push_back(0);
          this->runs[this->y].push_back(1);
        }
      }
      this->last_pixel = pixel;
    }
    void output_r4(std::ostream &);
    void output_pbm(std::ostream &);
    typedef BitmapIterator iterator;
    friend class BitmapIterator;
    iterator & begin() const;
  };

  class BitmapIterator
  {
  protected:
    const Bitmap &bitmap;
    unsigned int y, u, n;
  public:
    explicit BitmapIterator(const Bitmap &)
    : bitmap(bitmap),
      y(0), u(0), n(0)
    { }
    bool operator *()
    {
      return this->u & 1;
    }
    void operator ++(int)
    {
      this->n++;
      while (this->bitmap.runs[this->y][this->u] >= this->n)
      {
        this->u++;
        this->n = 0;
      }
    }
    void next_row()
    {
      this->y++;
      this->u = this-> n = 0;
    }
  };

  class FilterData
  {
  protected:
    const Config &config;
    pdf::Renderer &r1;
    pdf::Renderer &r2;
    unsigned int width, height;
    std::ostream &sep_stream;
  protected:
    explicit FilterData(
      const Config &config,
      pdf::Renderer &r1, pdf::Renderer &r2,
      unsigned int width, unsigned int height,
      std::ostream &sep_stream
    )
    : config(config),
      r1(r1), r2(r2),
      width(width), height(height),
      sep_stream(sep_stream)
    { }
    friend class Filter;
  };

  class Filter
  {
  protected:
    const Config &config;
    unsigned int width, height;
    bool black_foreground;
    bool solid_background;
    uint8_t background_color[3];
    std::ostream *sep_stream;
    std::auto_ptr<image::Bitmap> mask;
    std::auto_ptr<pdf::Pixmap> image;
    pdf::Renderer *r1, *r2;
    explicit Filter(const Config &config)
    : config(config)
    { }
  protected:
    virtual void extract_image(pdf::Renderer &);
    virtual void extract_mask(pdf::Renderer &, bool ignorable);
    virtual void process_image() = 0;
  public:
    virtual void filter(FilterData &);
    pdf::Pixmap * get_image()
    {
      return this->image.get();
    };
    image::Bitmap * get_mask()
    {
      return this->mask.get();
    }
    bool has_black_foreground() const
    {
      return this->black_foreground;
    }
    bool has_solid_background() const
    {
      return this->solid_background;
    }
    uint8_t *get_background_color()
    {
      return this->background_color;
    }
  };

  class DefaultFilter : public Filter
  {
    virtual void process_image();
  };

  class WebSafeFilter : public Filter
  {
    virtual void process_image();
  };

  class MaskFilter : public Filter
  {
    virtual void process_image();
  };

  class DummyFilter : public Filter
  {
    virtual void extract_image(pdf::Renderer &);
    virtual void extract_mask(pdf::Renderer &, bool ignorable);
    virtual void process_image();
  };

  class GraphicsMagickFilter : public Filter
  {
    virtual void process_image();
    class NotImplementedError : public std::runtime_error
    {
    public:
      NotImplementedError()
      : std::runtime_error(_("Advanced color quantization is not supported."))
      { };
    };
  };

}

#endif

// vim:ts=2 sw=2 et
