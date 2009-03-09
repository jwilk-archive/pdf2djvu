/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "debug.hh"

DevNull dev_null;

std::ostream &debug(int n, int threshold)
{
  if (n <= threshold)
    return std::clog;
  else
    return dev_null;
}

// vim:ts=2 sw=2 et
