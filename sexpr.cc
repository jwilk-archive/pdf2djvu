/* Copyright © 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "sexpr.hh"

static std::ostream *puts_stream = NULL;

static int my_puts(const char *s)
{
  *puts_stream << s;
  return -(!puts_stream->good());
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

// vim:ts=2 sts=2 sw=2 et
