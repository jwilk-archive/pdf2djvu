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
  DevNull()
  : std::ostream(0)
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

// vim:ts=2 sw=2 et
