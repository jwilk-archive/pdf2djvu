/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_DEBUG_H
#define PDF2DJVU_DEBUG_H

#include <iostream>
#include <stdexcept>
#include <sstream>

#include "system.hh"

class DebugStream;

template <typename tp>
static inline DebugStream &operator<<(DebugStream &, const tp &);

class DebugStream
{
protected:
  unsigned int level;
  bool started;
  std::ostream &ostream;
  void indent();
public:
  explicit DebugStream(std::ostream &ostream)
  : level(0), started(false), ostream(ostream)
  { }
  void operator ++(int) { this->level++; }
  void operator --(int) { this->level--; }
  template <typename tp>
    friend DebugStream &operator<<(DebugStream &, const tp &);
  friend DebugStream &operator<<(DebugStream &stream, std::ostream& (*)(std::ostream&));
};

DebugStream &debug(int n, int threshold);
extern DebugStream error_log;

extern std::ostream &dev_null;

static inline std::ostream &operator<<(std::ostream &stream, const std::runtime_error &error)
{
  stream << error.what();
  return stream;
}

template <typename tp>
static inline DebugStream &operator<<(DebugStream &stream, const tp &object)
{
  if (!stream.started)
  {
    stream.indent();
    stream.started = true;
  }
  std::ostringstream buffer;
  buffer.copyfmt(stream.ostream);
  buffer << object;
  std::string string = buffer.str();
  stream.ostream << encoding::proxy<encoding::native, encoding::terminal>(string);
  return stream;
}

#endif

// vim:ts=2 sw=2 et
