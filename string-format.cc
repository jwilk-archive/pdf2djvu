/* Copyright © 2009-2017 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
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

#include "string-format.hh"

#include <climits>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "i18n.hh"
#include "string-printf.hh"

namespace string_format
{

  class StaticChunk : public Chunk
  {
  protected:
    std::string value;
  public:
    StaticChunk(const std::string &value)
    : value(value)
    { }
    virtual ~StaticChunk()
    { }
    virtual void format(const Bindings &bindings, std::ostream &stream) const
    {
      stream << this->value;
    }
  };

  class VariableChunk : public Chunk
  {
  protected:
    std::string variable;
    int offset;
    unsigned int width;
    bool auto_width;
    bool pad_0;
  public:
    explicit VariableChunk(const std::string &);
    virtual ~VariableChunk()
    { }
    virtual void format(const Bindings &, std::ostream &) const;
  };

  class ValueError : public std::domain_error
  {
  public:
    ValueError(const std::string &what)
    : std::domain_error(what)
    { }
  };

  class ValueTypeError : public ValueError
  {
  public:
    ValueTypeError(const std::string &what)
    : ValueError(what)
    { }
  };

  class FormatError : public std::runtime_error
  {
  public:
    FormatError(const std::string &var, const std::string &what)
    : std::runtime_error(string_printf(_("Unable to format field {%s}: %s"), var.c_str(), what.c_str()))
    { }
  };

}

string_format::Value string_format::Bindings::get(const std::string &value) const
{
  const_iterator it = this->find(value);
  if (it == this->end())
    throw ValueError(_("no such variable"));
  return it->second;
}

string_format::VariableChunk::VariableChunk(const std::string &description)
: offset(0), width(0), auto_width(false), pad_0(false)
{
  enum
  {
    NAME,
    OFFSET_1,
    OFFSET_2,
    WIDTH_1,
    WIDTH_2,
    END
  } mode = NAME;
  std::string::const_iterator it = description.begin();
  int offset_sign = 1;
  while (it != description.end())
  {
    switch (mode)
    {
    case NAME:
      if (*it == '+' || *it == '-' || *it == ':')
      {
        this->variable = description.substr(0, it - description.begin());
        if (*it == ':')
          mode = WIDTH_1;
        else
        {
          offset_sign = (*it == '+') ? +1 : -1;
          mode = OFFSET_1;
        }
      }
      break;
    case OFFSET_1:
      if (*it >= '0' && *it <= '9')
      {
        this->offset = *it - '0';
        mode = OFFSET_2;
      }
      else
        throw string_format::ParseError();
      break;
    case OFFSET_2:
      if (*it >= '0' && *it <= '9')
      {
        if (this->offset > (INT_MAX - 9) / 10)
          throw string_format::FormatError(this->variable, _("integer overflow"));
        this->offset = this->offset * 10 + (*it - '0');
      }
      else if (*it == ':')
        mode = WIDTH_1;
      else
        throw string_format::ParseError();
      break;
    case WIDTH_1:
      if (*it == '0')
        this->pad_0 = true;
      else if (*it >= '0' && *it <= '9')
        this->width = *it - '0';
      else
        throw string_format::ParseError();
      mode = WIDTH_2;
      break;
    case WIDTH_2:
      if (*it >= '0' && *it <= '9')
      {
        if (this->width > (UINT_MAX - 9) / 10)
          throw string_format::FormatError(this->variable, _("integer overflow"));
        this->width = this->width * 10 + (*it - '0');
      }
      else if (*it == '*')
      {
        this->auto_width = true;
        mode = END;
      }
      else
        throw string_format::ParseError();
      break;
    case END:
      throw string_format::ParseError();
    }
    it++;
  }
  if (mode == NAME)
    this->variable = description;
  this->offset *= offset_sign;
}

unsigned int string_format::Value::as_int(int offset)
{
  if (!this->is_int)
    throw string_format::ValueTypeError(_("type error: expected number, not string"));
  if (offset < 0)
  {
    unsigned int uoffset = -offset;
    if (uoffset > this->v_uint)
      return 0;
    else
      return this->v_uint - uoffset;
  }
  else
  {
    unsigned int uoffset = offset;
    if (this->v_uint + uoffset >= this->v_uint)
      return this->v_uint + uoffset;
    else
      throw string_format::ValueError(_("integer overflow"));
  }
}

const std::string & string_format::Value::as_string()
{
  if (this->is_int)
    throw string_format::ValueTypeError(_("type error: expected string, not number"));
  return this->v_string;
}

void string_format::VariableChunk::format(const Bindings &bindings, std::ostream &stream) const
try
{
  Value value = bindings.get(this->variable);
  unsigned int width = this->width;
  if (this->auto_width)
  {
    unsigned int max_value;
    try
    {
      max_value = bindings.get("max_" + this->variable).as_int(this->offset);
    }
    catch (const string_format::ValueError &)
    {
      throw FormatError(this->variable, _("unknown maximum width"));
    }
    unsigned int max_digits = 0;
    while (max_value > 0)
    {
      max_digits++;
      max_value /= 10;
    }
    if (max_digits > width)
      width = max_digits;
  }
  stream
    << std::setfill(this->pad_0 ? '0' : ' ')
    << std::setw(width);
  try
  {
    stream << value.as_int(this->offset);
  }
  catch (const string_format::ValueTypeError &)
  {
    if (this->offset != 0)
      throw;
    stream << value.as_string();
  }
}
catch (const string_format::ValueError &exc)
{
  throw FormatError(this->variable, exc.what());
}

string_format::Template::Template(const std::string &source)
{
  enum
  {
    TEXT,
    KET,
    FORMAT_1,
    FORMAT_2
  } mode = TEXT;
  std::string::const_iterator left = source.begin();
  std::string::const_iterator right = left;
  while (right != source.end())
  {
    switch (mode)
    {
    case TEXT:
      if (*right == '{' || *right == '}')
      {
        if (left != right)
        {
          std::string text = source.substr(left - source.begin(), right - left);
          this->chunks.push_back(new StaticChunk(text));
        }
        left = right + 1;
        mode = *right == '}' ? KET : FORMAT_1;
      }
      break;
    case KET:
      if (*right == '}')
      {
        left = right;
        mode = TEXT;
      }
      else
        throw string_format::ParseError();
      break;
    case FORMAT_1:
      if (*right == '{')
      {
        left = right;
        mode = TEXT;
      }
      else
        mode = FORMAT_2;
      break;
    case FORMAT_2:
      if (*right == '}')
      {
        std::string text = source.substr(left - source.begin(), right - left);
        this->chunks.push_back(new VariableChunk(text));
        left = right + 1;
        mode = TEXT;
      }
      break;
    }
    right++;
  }
  if (mode != TEXT)
    throw string_format::ParseError();
  if (left != source.end())
  {
    std::string text = source.substr(left - source.begin(), std::string::npos);
    this->chunks.push_back(new StaticChunk(text));
  }
}

string_format::Template::~Template()
{
  for (auto &chunk : this->chunks)
    delete chunk;
}

void string_format::Template::format(const Bindings &bindings, std::ostream &stream) const
{
  for (const Chunk* chunk : this->chunks)
    (*chunk).format(bindings, stream);
}

std::string string_format::Template::format(const Bindings &bindings) const
{
  std::ostringstream stream;
  this->format(bindings, stream);
  return stream.str();
}

// vim:ts=2 sts=2 sw=2 et
