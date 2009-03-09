/* Copyright © 2008, 2009 Jakub Wilk
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

  class GCLock
  {
  public:
    GCLock() { minilisp_acquire_gc_lock(0); }
    ~GCLock() throw () { minilisp_release_gc_lock(0); }
  };
}

#endif

// vim:ts=2 sw=2 et
