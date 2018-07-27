/* Copyright © 2008-2018 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_SEXPR_H
#define PDF2DJVU_SEXPR_H

#include <iostream>
#include <string>

#include <libdjvu/miniexp.h>

namespace sexpr
{
  typedef miniexp_t Expr;
  class Ref
  {
    minivar_t var;
  public:
    Ref() : var(miniexp_nil) { }
    Ref(const Ref &ref) : var(ref.var) { }
    Ref(miniexp_t expr) : var(expr) { }
    Ref& operator =(miniexp_t expr) { this->var = expr; return *this; }
    operator Expr&() const { return const_cast<Ref*>(this)->var; }
    inline void reverse()
    {
      this->var = miniexp_reverse(this->var);
    }
    friend std::ostream &operator<<(std::ostream &, const Ref &);
  };
  static inline Expr cons(Expr car, Expr cdr) { return miniexp_cons(car, cdr); }
  static inline Expr symbol(const char *name) { return miniexp_symbol(name); }
  static inline Expr symbol(const std::string &name) { return miniexp_symbol(name.c_str()); }
  static inline Expr string(const char *value) { return miniexp_string(value); }
  static inline Expr string(const std::string &value) { return miniexp_string(value.c_str()); }
  static inline Expr integer(int n) { return miniexp_number(n); }
  static const Expr nil = miniexp_nil;
  static const Ref &empty_string = string("");

  class Guard
  {
  public:
      Guard();
      ~Guard();
  };

}

#endif

// vim:ts=2 sts=2 sw=2 et
