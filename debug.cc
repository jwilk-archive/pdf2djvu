/* Copyright © 2007-2016 Jakub Wilk <jwilk@jwilk.net>
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

#include "debug.hh"

#include <iostream>

class DevNull : public std::ostream
{
public:
  DevNull()
  : std::ostream(nullptr)
  { }
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

void DebugStream::indent()
{
  unsigned int level = this->level;
  if (level > 0)
  {
    while (level-- > 1)
      this->ostream << "  ";
    this->ostream << "- ";
  }
}

DebugStream &operator<<(DebugStream &stream, std::ostream& (*pf)(std::ostream&))
{
  if (pf == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
    stream.started = false;
  stream.ostream << pf;
  return stream;
}

// vim:ts=2 sts=2 sw=2 et
