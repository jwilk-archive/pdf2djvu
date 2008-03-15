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
  class Ref
  {
    minivar_t var;
  public:
    Ref(const Ref &ref) { this->var = ref.var; }
    Ref(miniexp_t expr) { this->var = expr; }
    Ref& operator =(miniexp_t expr) { this->var = expr; return *this; }
    operator miniexp_t&() const { return const_cast<Ref*>(this)->var; }
    inline const char * repr()
    {
      return miniexp_to_str(miniexp_pname(this->var, 0));
    }
    friend std::ostream &operator<<(std::ostream &, const Ref &);
  };
  static inline miniexp_t cons(miniexp_t car, miniexp_t cdr) { return miniexp_cons(car, cdr); }
  static inline miniexp_t symbol(const char *name) { return miniexp_symbol(name); }
  static inline miniexp_t symbol(const std::string &name) { return miniexp_symbol(name.c_str()); }
  static inline miniexp_t string(const char *value) { return miniexp_string(value); }
  static inline miniexp_t string(const std::string &value) { return miniexp_string(value.c_str()); }
  static inline miniexp_t integer(int n) { return miniexp_number(n); }
  static const miniexp_t nil = miniexp_nil;
  static const Ref &empty_string = string("");
}

#endif

// vim:ts=2 sw=2 et
