/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "string-format.hh"

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
    virtual ~StaticChunk() throw ()
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
    bool negative_offset;
    uint_tp offset;
    unsigned int width;
    bool auto_width;
    bool pad_0;
  public:
    explicit VariableChunk(const std::string &);
    virtual ~VariableChunk() throw ()
    { }
    virtual void format(const Bindings &, std::ostream &) const;
  };

  class IntegerOverflow : public std::overflow_error
  {
  public:
    IntegerOverflow()
    : std::overflow_error("Integer overflow")
    { }
  };

}

string_format::VariableChunk::VariableChunk(const std::string &description)
: negative_offset(false), offset(0), width(0), auto_width(false), pad_0(false) 
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
          this->negative_offset = *it == '-';
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
        throw ParseError();
      break;
    case OFFSET_2:
      if (*it >= '0' && *it <= '9')
      {
        if (this->offset > (std::numeric_limits<uint_tp>::max() - 9) / 10)
          throw IntegerOverflow();
        this->offset = this->offset * 10 + (*it - '0');
      }
      else if (*it == ':')
        mode = WIDTH_1;
      else
        throw ParseError();
      break;
    case WIDTH_1:
      if (*it == '0')
        this->pad_0 = true;
      else if (*it >= '0' && *it <= '9')
        this->width = *it - '0';
      else
        throw ParseError();
      mode = WIDTH_2;
      break;
    case WIDTH_2:
      if (*it >= '0' && *it <= '9')
      {
        if (this->width > (std::numeric_limits<typeof (this->width)>::max() - 9) / 10)
          throw IntegerOverflow();
        this->width = this->width * 10 + (*it - '0');
      }
      else if (*it == '*')
      {
        this->auto_width = true;
        mode = END;
      }
      else
        throw ParseError();
      break;
    case END:
      throw ParseError();
      break;
    }
    it++;
  }
  if (mode == NAME)
    this->variable = description;
}

static string_format::uint_tp shift(string_format::uint_tp value, string_format::uint_tp offset, bool negative_offset)
{
  if (negative_offset)
  {
    if (offset > value)
      value = 0;
    else
      value -= offset;
  }
  else if (value + offset >= value)
    value += offset;
  else
    throw string_format::IntegerOverflow();
  return value;
}

void string_format::VariableChunk::format(const Bindings &bindings, std::ostream &stream) const
{
  uint_tp value = shift(bindings.get(this->variable), this->offset, this->negative_offset);
  unsigned int width = this->width;
  if (auto_width)
  {
    uint_tp max_value = shift(bindings.get("max_" + this->variable), this->offset, this->negative_offset);
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
    << std::setw(width)
    << value;
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
        throw ParseError();
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
    throw ParseError();
  if (left != source.end())
  {
    std::string text = source.substr(left - source.begin(), std::string::npos);
    this->chunks.push_back(new StaticChunk(text));
  }
}

string_format::Template::~Template() throw ()
{
  typedef std::vector<Chunk*>::iterator iterator;
  for (iterator it = this->chunks.begin(); it != this->chunks.end(); it++)
    delete *it;
}

void string_format::Template::format(const Bindings &bindings, std::ostream &stream) const
{
  typedef std::vector<Chunk*>::const_iterator iterator;
  for (iterator it = this->chunks.begin(); it != this->chunks.end(); it++)
    (**it).format(bindings, stream);
}

std::string string_format::Template::format(const Bindings &bindings) const
{
  std::ostringstream stream;
  this->format(bindings, stream);
  return stream.str();
}

// vim:ts=2 sw=2 et
