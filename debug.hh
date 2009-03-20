/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <iostream>
#include <stdexcept>

#ifndef PDF2DJVU_DEBUG_H
#define PDF2DJVU_DEBUG_H

class DebugStream;

template <typename tp>
static inline DebugStream &operator<<(DebugStream &, const tp &object);
static inline DebugStream &operator<<(DebugStream &stream, std::ostream& (*)(std::ostream&));

class DebugStream
{
protected:
  unsigned int indent;
  bool started;
  std::ostream &ostream;
public:
  explicit DebugStream(std::ostream &ostream)
  : indent(0), started(false), ostream(ostream)
  { }
  void operator ++(int) { this->indent++; }
  void operator --(int) { this->indent--; }
  template <typename tp>
    friend DebugStream &operator<<(DebugStream &, const tp &);
  friend DebugStream &operator<<(DebugStream &stream, std::ostream& (*)(std::ostream&));
};

template <typename tp>
static inline DebugStream &operator<<(DebugStream &stream, const tp &object)
{
  if (!stream.started)
  {
    unsigned int indent = stream.indent;
    if (indent > 0)
    {
      while (indent-- > 1)
        stream.ostream << "  ";
      stream.ostream << "- ";
    }
    stream.started = true;
  }
  stream.ostream << object;
  return stream;
}

static inline DebugStream &operator<<(DebugStream &stream, std::ostream& (*pf)(std::ostream&))
{
  if (pf == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
    stream.started = false;
  stream.ostream << pf;
  return stream;
}

DebugStream &debug(int n, int threshold);
extern DebugStream error_log;

extern std::ostream &dev_null;

static inline std::ostream &operator<<(std::ostream &stream, const std::runtime_error &error)
{
  stream << error.what();
  return stream;
}

#endif

// vim:ts=2 sw=2 et
