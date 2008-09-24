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
#include <stdexcept>

#include "sexpr.hh"

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
  extern bool text_nfkc;
  extern std::string output;
  extern bool output_stdout;
  extern int verbose;
  extern int dpi;
  extern std::pair<int, int> preferred_page_size;
  extern bool use_media_box;
  extern int bg_subsample;
  extern int fg_colors;
  extern bool monochrome;
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
  extern std::string pageid_prefix;

  class Error : public std::runtime_error
  {
  public:
    explicit Error(const std::string &message) : std::runtime_error(message) {};
    virtual bool is_quiet() const
    {
      return false;
    }
    virtual bool is_already_printed() const
    {
      return false;
    }
    virtual ~Error() throw() { /* just to shut up compilers */ }
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

  class NoPagesSelected : public Error
  {
  public:
    NoPagesSelected() : Error("No pages selected") { }
  };

  class HyperlinksOptionsParseError : public Error
  {
  public:
    HyperlinksOptionsParseError() : Error("Unable to parse hyperlinks options") {}
  };

  class DpiOutsideRange : public Error
  {
  private:
    static std::string __error_message__(int dpi_from, int dpi_to)
    {
      std::ostringstream stream;
      stream
        << "The specified resolution is outside the allowed range: " 
        << dpi_from << " .. " << dpi_to;
      return stream.str();
    }
  public:
    DpiOutsideRange(int dpi_from, int dpi_to) 
    : Error(__error_message__(dpi_from, dpi_to))
    { }
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

  class NeedVersion
  { };

  void read_config(int argc, char * const argv[]);
  void usage(const Error &error);
}

#endif

// vim:ts=2 sw=2 et
