/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <map>

namespace string_format
{
  typedef unsigned long uint_tp;

  class Bindings : public std::map<std::string, uint_tp>
  {
  public:
    uint_tp get(const std::string &value) const
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

// vim:ts=2 sw=2 et
