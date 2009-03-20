/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "debug.hh"

class DevNull : public std::ostream
{
public:
  DevNull() : std::ostream(0) { }
};

static DevNull static_dev_null;
std::ostream &dev_null = static_dev_null;

DebugStream error_log(std::cerr);
static DebugStream null_debug(dev_null);
static DebugStream full_debug(std::clog);

DebugStream &debug(int n, int threshold)
{
  if (n <= threshold)
    return full_debug;
  else
    return null_debug;
}

// vim:ts=2 sw=2 et
