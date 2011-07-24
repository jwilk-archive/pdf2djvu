/* Copyright © 2007, 2008, 2009, 2010 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_CONFIG_H
#define PDF2DJVU_CONFIG_H

#include <climits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "i18n.hh"
#include "sexpr.hh"
#include "string-format.hh"

class Config
{
protected:
  static string_format::Template *default_pageid_template(const std::string &pageid_prefix);
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

  enum
  {
    FG_COLORS_DEFAULT = UINT_MAX,
    FG_COLORS_WEB = UINT_MAX - 1,
    FG_COLORS_BLACK = UINT_MAX - 2,
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
  bool guess_dpi;
  std::pair<int, int> preferred_page_size;
  bool use_media_box;
  unsigned int bg_subsample;
  unsigned int fg_colors;
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
  std::vector<const char*> filenames;
  std::auto_ptr<string_format::Template> pageid_template;
  std::auto_ptr<string_format::Template> page_title_template;
  std::string text_filter_command_line;
  int n_jobs;

  Config();

  class NeedVersion
  { };

  class Error : public std::runtime_error
  {
  public:
    explicit Error(const std::string &message)
    : std::runtime_error(message)
    { };
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

  class NoPagesSelected : public Error
  {
  public:
    NoPagesSelected()
    : Error(_("No pages selected"))
    { }
  };

  class NeedHelp : public Error
  {
  public:
    NeedHelp()
    : Error("")
    { };
    virtual bool is_quiet() const
    {
      return true;
    }
  };

  class InvalidOption : public Error
  {
  public:
    InvalidOption()
    : Error("")
    { };
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
  void usage(const Error &error) const;
};

#endif

// vim:ts=2 sw=2 et
