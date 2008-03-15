/* Copyright © 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_SEXPR_H
#define PDF2DJVU_SEXPR_H

#include <iostream>

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
    inline std::string repr()
    {
      Ref pname = miniexp_pname(this->var, 0);
      std::string result = miniexp_to_str(pname);
      return result;
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
}

#endif

// vim:ts=2 sw=2 et
