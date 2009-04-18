/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_CONFIG_H
#define PDF2DJVU_CONFIG_H

#include <sstream> 
#include <stdexcept>
#include <string>
#include <vector>

#include "sexpr.hh"


class Config
{
public:
  class Hyperlinks
  {
  public:
    bool extract;
    bool border_always_visible;
    std::string border_color;
    Hyperlinks()
    : extract(true), border_always_visible(false)
    { };
  };

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
  format_t format;
  text_t text;
  bool text_nfkc;
  bool text_crop;
  std::string output;
  bool output_stdout;
  int verbose;
  int dpi;
  std::pair<int, int> preferred_page_size;
  bool use_media_box;
  int bg_subsample;
  int fg_colors;
  bool monochrome;
  int loss_level;
  bool antialias;
  Hyperlinks hyperlinks;
  bool extract_metadata;
  bool adjust_metadata;
  bool extract_outline;
  bool no_render;
  char *bg_slices;
  std::vector< std::pair<int, int> > pages;
  char *file_name;
  std::string pageid_prefix;

  Config();

  class NeedVersion
  { };

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
    virtual ~Error() throw () { /* just to shut up compilers */ }
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

  void read_config(int argc, char * const argv[]);
  void usage(const Error &error);
};

#endif

// vim:ts=2 sw=2 et
