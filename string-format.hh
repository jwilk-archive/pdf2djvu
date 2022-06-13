/* Copyright © 2009-2022 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_STRING_FORMAT_HH
#define PDF2DJVU_STRING_FORMAT_HH

#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace string_format
{

  class Value
  {
  protected:
    bool is_int;
    unsigned int v_uint;
    std::string v_string;
  public:
    Value(unsigned int n=0)
    : is_int(true), v_uint(n)
    { }
    Value(const std::string &s)
    : is_int(false), v_uint(0), v_string(s)
    { }
    Value(const char *s)
    : is_int(false), v_uint(0), v_string(s)
    { }
    unsigned int as_int(int offset=0);
    const std::string &as_string();
  };

  class Bindings : public std::map<std::string, Value>
  {
  public:
    Value get(const std::string &value) const;
  };

  class Chunk
  {
  public:
    virtual void format(const Bindings &bindings, std::ostream &stream) const
    = 0;
    virtual ~Chunk()
    { }
  };

  class Template
  {
  private:
    Template(const Template &) = delete;
    Template & operator=(const Template &) = delete;
  protected:
    std::vector<Chunk*> chunks;
  public:
    explicit Template(const std::string &);
    ~Template();
    void format(const Bindings &, std::ostream &) const;
    std::string format(const Bindings &) const;
  };

  class ParseError : public std::runtime_error
  {
  public:
    explicit ParseError()
    : std::runtime_error("")
    { }
  };

}

#endif

// vim:ts=2 sts=2 sw=2 et
