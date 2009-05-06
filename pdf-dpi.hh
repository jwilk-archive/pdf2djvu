/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
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
      double _min, _max;
    public:
      explicit Guess(double min, double max)
      : _min(min), _max(max)
      { };
      double min() const { return this->_min; }
      double max() const { return this->_max; }
    };

    class NoGuess
    { };

    class Guesser
    {
    protected:
      void *_magic;
      pdf::Document &_document;
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
};

#endif

// vim:ts=2 sw=2 et
