/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_CONFIG_H
#define PDF2DJVU_CONFIG_H

#include <algorithm>
#include <sstream> 
#include <string>
#include <vector>

#include "sexpr.hh"
#include "debug.hh"

namespace config
{
  enum text_t
  {
    TEXT_NONE = 0,
    TEXT_WORDS,
    TEXT_LINES
  };
  enum format_t
  {
    FORMAT_BUNDLED,
    FORMAT_INDIRECT
  };
  extern format_t format;
  extern text_t text;
  extern std::string output;
  extern bool output_stdout;
  extern int verbose;
  extern int dpi;
  extern std::pair<int, int> preferred_page_size;
  extern int bg_subsample;
  extern bool antialias;
  extern std::vector<sexpr::Ref> hyperlinks_options;
  extern bool hyperlinks_user_border_color;
  extern bool extract_hyperlinks;
  extern bool extract_metadata;
  extern bool extract_outline;
  extern bool no_render;
  extern char *bg_slices;
  extern std::vector< std::pair<int, int> > pages;
  extern char *file_name;
 
  class Error : public ::Error
  {
  public:
    explicit Error(const std::string &message) : ::Error(message) {};
    virtual bool is_quiet() const
    {
      return false;
    }
    virtual bool is_already_printed() const
    {
      return false;
    }
    virtual ~Error() { /* just to shut up compilers */ }
  };

  class PagesParseError : public Error
  {
  public:
    PagesParseError() : Error("Unable to parse page numbers") {}
  };

  class PageSizeParseError : public Error
  {
  public:
    PageSizeParseError() : Error("Unable to parse page size") {}
  };

  class HyperlinksOptionsParseError : public Error
  {
  public:
    HyperlinksOptionsParseError() : Error("Unable to parse hyperlinks options") {}
  };

  class DpiOutsideRange : public Error
  {
  public:
    DpiOutsideRange(int dpi_from, int dpi_to) : Error("The specified resolution is outside the allowed range")
    {
      std::ostringstream stream;
      stream << ": " << dpi_from << " .. " << dpi_to;
      this->message += stream.str();
    }
  };

  class NeedHelp : public Error
  {
  public:
    NeedHelp() : Error("") {};
    virtual bool is_quiet() const
    {
      return true;
    }
  };

  class InvalidOption : public Error
  {
  public:
    InvalidOption() : Error("") {};
    virtual bool is_quiet() const
    {
      return true;
    }
    virtual bool is_already_printed() const
    {
      return true;
    }
  };

  class NeedVersion : public ::Error
  {
  public:
    NeedVersion() : Error("") {};
  };

  void read_config(int argc, char * const argv[]);
  void usage(const Error &error);
}

#endif

// vim:ts=2 sw=2 et
