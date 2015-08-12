/* Copyright © 2009 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdfdjvu.
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

#ifndef PDF2DJVU_XMP_H
#define PDF2DJVU_XMP_H

#include <string>
#include <stdexcept>

#include "pdf-backend.hh"

namespace xmp
{
  std::string transform(const std::string &data, const pdf::Metadata &metadata);

  class XmlError : public std::runtime_error
  {
  public:
    explicit XmlError(const std::string message)
    : std::runtime_error(message)
    { }
  };
}

#endif

// vim:ts=2 sts=2 sw=2 et
