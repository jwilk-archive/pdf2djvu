/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "debug.h"
#include "config.h"

DevNull dev_null;

std::ostream &debug(int n)
{
  if (n <= config::verbose)
    return std::clog;
  else
    return dev_null;
}

std::ostream &operator<<(std::ostream &stream, const Error &error)
{
  return stream << error.message;
}

// vim:ts=2 sw=2 et
