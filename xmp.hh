/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
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

// vim:ts=2 sw=2 et
