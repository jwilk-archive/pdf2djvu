/* Copyright © 2007-2019 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
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

#ifndef PDF2DJVU_CONFIG_H
#define PDF2DJVU_CONFIG_H

#include <climits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "i18n.hh"
#include "string-format.hh"

class Config
{
protected:
  static string_format::Template *default_page_id_template(const std::string &page_id_prefix);
public:
  class Hyperlinks
  {
  public:
    bool extract;
    bool border_always_visible;
    std::string border_color;
    Hyperlinks()
    : extract(true), border_always_visible(false)
    { }
  };

  enum
  {
    FG_COLORS_DEFAULT = INT_MIN,
    FG_COLORS_WEB = INT_MIN + 1,
    FG_COLORS_BLACK = INT_MIN + 2,
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
  std::vector<std::pair<int, int>> pages;
  std::vector<const char*> filenames;
  std::unique_ptr<string_format::Template> page_id_template;
  std::unique_ptr<string_format::Template> page_title_template;
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
    { }
    virtual bool is_quiet() const
    {
      return false;
    }
    virtual bool is_already_printed() const
    {
      return false;
    }
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
    { }
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
    { }
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
  void usage() const;
};

#endif

// vim:ts=2 sts=2 sw=2 et
