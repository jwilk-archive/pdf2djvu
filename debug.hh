/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <iostream>
#include <stdexcept>

#ifndef PDF2DJVU_DEBUG_H
#define PDF2DJVU_DEBUG_H

std::ostream &debug(int n, int threshold);

class DevNull : public std::ostream
{
public:
  DevNull() : std::ostream(0) { }
};

extern DevNull dev_null;

static inline std::ostream &operator<<(std::ostream &stream, const std::runtime_error &error)
{
  return stream << error.what();
}

#endif

// vim:ts=2 sw=2 et
