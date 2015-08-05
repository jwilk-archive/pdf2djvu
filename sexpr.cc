/* Copyright © 2008-2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <cstdio>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

#include "sexpr.hh"

#if DDJVUAPI_VERSION >= 21

static int my_puts(miniexp_io_t* io, const char *s)
{
  std::ostream *puts_stream = reinterpret_cast<std::ostream*>(io->data[0]);
  *puts_stream << s;
  return puts_stream->good() ? 0 : EOF;
}

namespace sexpr
{
  std::ostream &operator<<(std::ostream &stream, const sexpr::Ref &expr)
  {
    miniexp_io_t io;
    miniexp_io_init(&io);
    io.fputs = my_puts;
    io.data[0] = &stream;
    miniexp_pprin_r(&io, expr, 1 << 10);
    return stream;
  }
}

#else

static std::ostream *puts_stream = NULL;

static int my_puts(const char *s)
{
  *puts_stream << s;
  return puts_stream->good() ? 0 : EOF;
}

namespace sexpr
{
  std::ostream &operator<<(std::ostream &stream, const sexpr::Ref &expr)
  {
    int (*old_minilisp_puts)(const char *s) = minilisp_puts;
    minilisp_puts = my_puts;
    puts_stream = &stream;
    miniexp_pprin(expr, 1 << 10);
    minilisp_puts = old_minilisp_puts;
    return stream;
  }
}

#endif

// vim:ts=2 sts=2 sw=2 et
