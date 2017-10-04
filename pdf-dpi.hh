/* Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_PDF_DPI_H
#define PDF2DJVU_PDF_DPI_H

#include "pdf-backend.hh"
#include <ostream>

namespace pdf
{
  namespace dpi
  {

    class Guess
    {
    protected:
      double min_, max_;
    public:
      explicit Guess(double min, double max)
      : min_(min), max_(max)
      { }
      double min() const { return this->min_; }
      double max() const { return this->max_; }
    };

    class NoGuess
    { };

    class Guesser
    {
    protected:
      void *magic;
      pdf::Document &document;
    public:
      explicit Guesser(pdf::Document &document);
      ~Guesser();
      Guess operator[](int n);
    };

  }

}

static inline std::ostream & operator <<(std::ostream &stream, const pdf::dpi::Guess &guess)
{
  stream << guess.min();
  if (guess.min() < guess.max())
    stream << ".." << guess.max();
  return stream;
}

#endif

// vim:ts=2 sts=2 sw=2 et
