/* Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdfdjvu.
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
#include <utility>
#include <vector>

namespace string_format
{

  class Bindings : public std::map<std::string, unsigned int>
  {
  public:
    unsigned int get(const std::string &value) const
    {
      const_iterator it = this->find(value);
      if (it == this->end())
        return 0;
      return it->second;
    }
  };

  class Chunk
  {
  public:
    virtual void format(const Bindings &bindings, std::ostream &stream) const
    = 0;
    virtual ~Chunk() throw ()
    { }
  };

  class Template
  {
  private:
    Template(const Template &); // not defined
    Template & operator=(const Template &); // not defined
  protected:
    std::vector<Chunk*> chunks;
  public:
    explicit Template(const std::string &);
    ~Template() throw ();
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
