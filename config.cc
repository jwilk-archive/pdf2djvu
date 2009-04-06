/* Copyright © 2007, 2008, 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>
#include <stdexcept>

#include <getopt.h>

#include "config.hh"

#include "debug.hh"
#include "djvuconst.hh"
#include "version.hh"

Config::Config()
{
  this->text = this->TEXT_WORDS;
  this->text_nfkc = true;
  this->format = this->FORMAT_BUNDLED;
  this->output_stdout = true;
  this->verbose = 1;
  this->dpi = 300;
  this->preferred_page_size = std::make_pair(0, 0);
  this->use_media_box = false;
  this->bg_subsample = 3;
  this->fg_colors = -1;
  this->antialias = false;
  this->hyperlinks_user_border_color = false;
  this->extract_metadata = true;
  this->adjust_metadata = true;
  this->extract_outline = true;
  this->no_render = false;
  this->monochrome = false;
  this->loss_level = 0;
  this->bg_slices = NULL;
  this->file_name = NULL;
  this->pageid_prefix = "p";
}

namespace string
{
  static void split(const std::string &, char, std::vector<std::string> &);

  static void replace(std::string &, char, char);

  template <typename tp>
  tp as(const char *);
  
  template <>
  long as<long>(const char *);
  
  template <typename tp>
  tp as(const std::string &);
}

static void string::split(const std::string &s, char c, std::vector<std::string> &result)
{
  size_t lpos = 0;
  while (true)
  {
    size_t rpos = s.find(c, lpos);
    result.push_back(s.substr(lpos, rpos - lpos));
    if (rpos == std::string::npos)
      break;
    else
      lpos = rpos + 1;
  }
}

static void string::replace(std::string &s, char c1, char c2)
{
  size_t lpos = 0;
  while (true)
  {
    size_t rpos = s.find(c1, lpos);
    if (rpos == std::string::npos)
      break;
    s[rpos] = c2;
    lpos = rpos + 1;
  }
}

static void parse_hyperlinks_options(std::string s, Config::Hyperlinks &options)
{
  std::vector<std::string> splitted;
  string::replace(s, '_', '-');
  string::split(s, ',', splitted);
  for (std::vector<std::string>::const_iterator it = splitted.begin(); it != splitted.end(); it++)
  {
    if (*it == "border-avis")
    {
      options.border_always_visible = true;
      continue;
    }
    else if (*it == "no" || *it == "none")
    {
      options.extract = false;
      continue;
    }
    else if 
    (
      it->length() == 7 &&
      (*it)[0] == '#' &&
      it->find_first_not_of("0123456789abcdefABCDEF", 1) == std::string::npos
    )
    {
      options.border_color = *it;
      continue;
    }
    throw Config::HyperlinksOptionsParseError();
  }
}

static void parse_pages(const std::string &s, std::vector< std::pair<int, int> > &result)
{
  int state = 0;
  int value[2] = { 0, 0 };
  for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
  {
    if (('0' <= *it) && (*it <= '9'))
    {
      value[state] = value[state] * 10 + (int)(*it - '0');
      if (state == 0)
        value[1] = value[0];
    }
    else if (state == 0 && *it == '-')
    {
      state = 1;
      value[1] = 0;
    }
    else if (*it == ',')
    {
      if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
        throw Config::PagesParseError();
      result.push_back(std::make_pair(value[0], value[1]));
      value[0] = value[1] = 0;
      state = 0;
    }
    else
      throw Config::PagesParseError();
  }
  if (state == 0)
    value[1] = value[0];
  if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
    throw Config::PagesParseError();
  result.push_back(std::make_pair(value[0], value[1]));
}

static std::pair<int, int>parse_page_size(const std::string &s)
{
  std::istringstream stream(s);
  int x, y;
  char c;
  stream >> x >> c >> y;
  if (x > 0 &&  y > 0 && c == 'x' && stream.eof() && !stream.fail())
    return std::make_pair(x, y);
  else
    throw Config::PageSizeParseError();
}

class InvalidNumber : public Config::Error
{
private:
  static std::string __error_message__(const char *s)
  {
    std::ostringstream stream;
    stream
      << s << " is not a valid number";
    return stream.str();
  }
public:
  explicit InvalidNumber(const char *s)
  : Config::Error(__error_message__(s))
  { }
};

template <typename tp>
tp string::as(const char *s)
{
  long result = as<long>(s);
  if (result > std::numeric_limits<tp>::max())
    throw InvalidNumber(s);
  if (result < std::numeric_limits<tp>::min())
    throw InvalidNumber(s);
  return result;
}

template <>
long string::as<long>(const char *s)
{
  char *end_ptr;
  long result;
  errno = 0;
  result = strtol(s, &end_ptr, 10);
  if (*end_ptr != '\0' || errno == ERANGE)
    throw InvalidNumber(s);
  return result;
}

template <typename tp>
tp string::as(const std::string &s)
{
  return as<tp>(s.c_str());
}

static int parse_fg_colors(const std::string &s)
{
  if (s == "web")
    return -1;
  long n = string::as<long>(s);
  if (n < 1 || n > djvu::max_fg_colors)
    throw Config::Error("The specified number of foreground colors is outside the allowed range");
  return n;
}

static const std::string& parse_pageid_prefix(const std::string &s)
{
  for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
  {
    if (isalnum(*it) || *it == '_' || *it == '-' || *it == '+' || *it == '.')
      ;
    else
      throw Config::Error("Pageid prefix must consist only of letters, digits, '_', '+', '-' and '.' characters");
  }
  return s;
}

void Config::read_config(int argc, char * const argv[])
{
  enum
  {
    OPT_DPI = 'd',
    OPT_HELP = 'h',
    OPT_INDIRECT = 'i',
    OPT_OUTPUT = 'o',
    OPT_PAGES = 'p',
    OPT_QUIET = 'q',
    OPT_VERBOSE = 'v',
    OPT_DUMMY = CHAR_MAX,
    OPT_ANTIALIAS,
    OPT_BG_SLICES,
    OPT_BG_SUBSAMPLE,
    OPT_FG_COLORS,
    OPT_HYPERLINKS,
    OPT_LOSS_100,
    OPT_LOSS_ANY,
    OPT_MEDIA_BOX,
    OPT_MONOCHROME,
    OPT_NO_HLINKS,
    OPT_NO_METADATA,
    OPT_NO_OUTLINE,
    OPT_NO_RENDER,
    OPT_PAGE_SIZE,
    OPT_PREFIX,
    OPT_TEXT_LINES,
    OPT_TEXT_NONE,
    OPT_TEXT_WORDS,
    OPT_TEXT_NO_NFKC,
    OPT_VERBATIM_METADATA,
    OPT_VERSION,
  };
  static struct option options [] =
  {
    { "anti-alias", 0, 0, OPT_ANTIALIAS },
    { "antialias", 0, 0, OPT_ANTIALIAS },
    { "bg-slices", 1, 0, OPT_BG_SLICES },
    { "bg-subsample", 1, 0, OPT_BG_SUBSAMPLE },
    { "dpi", 1, 0, OPT_DPI },
    { "fg-colors", 1, 0, OPT_FG_COLORS },
    { "help", 0, 0, OPT_HELP },
    { "hyperlinks", 1, 0, OPT_HYPERLINKS },
    { "indirect", 0, 0, OPT_INDIRECT },
    { "lines", 0, 0, OPT_TEXT_LINES },
    { "loss-level", 1, 0, OPT_LOSS_ANY },
    { "losslevel", 1, 0, OPT_LOSS_ANY },
    { "lossy", 0, 0, OPT_LOSS_100 },
    { "media-box", 0, 0, OPT_MEDIA_BOX },
    { "monochrome", 0, 0, OPT_MONOCHROME },
    { "no-hyperlinks", 0, 0, OPT_NO_HLINKS },
    { "no-metadata", 0, 0, OPT_NO_METADATA },
    { "no-outline", 0, 0, OPT_NO_OUTLINE },
    { "no-render", 0, 0, OPT_NO_RENDER },
    { "no-text", 0, 0, OPT_TEXT_NONE },
    { "no-nfkc", 0, 0, OPT_TEXT_NO_NFKC },
    { "output", 1, 0, OPT_OUTPUT },
    { "page-size", 1, 0, OPT_PAGE_SIZE },
    { "pageid-prefix", 1, 0, OPT_PREFIX },
    { "pages", 1, 0, OPT_PAGES },
    { "quiet", 0, 0, OPT_QUIET },
    { "verbatim-metadata", 0, 0, OPT_VERBATIM_METADATA },
    { "verbose", 0, 0, OPT_VERBOSE },
    { "version", 0, 0, OPT_VERSION },
    { "words", 0, 0, OPT_TEXT_WORDS },
    { NULL, 0, 0, '\0' }
  };
  int optindex, c;
  while (true)
  {
    optindex = 0;
    c = getopt_long(argc, argv, "i:o:d:qvp:h", options, &optindex);
    if (c < 0)
      break;
    if (c == 0)
      throw Error("Unable to parse command-line options");
    switch (c)
    {
    case OPT_DPI:
      this->dpi = string::as<int>(optarg);
      if (this->dpi < djvu::min_dpi || this->dpi > djvu::max_dpi)
        throw Config::DpiOutsideRange(djvu::min_dpi, djvu::max_dpi);
      break;
    case OPT_PAGE_SIZE:
      this->preferred_page_size = parse_page_size(optarg);
      break;
    case OPT_MEDIA_BOX:
      this->use_media_box = true;
      break;
    case OPT_QUIET:
      this->verbose = 0;
      break;
    case OPT_VERBOSE:
      this->verbose++;
      break;
    case OPT_BG_SLICES:
      this->bg_slices = optarg;
      break;
    case OPT_BG_SUBSAMPLE:
      this->bg_subsample = string::as<int>(optarg);
      if (this->bg_subsample < 1 || this->bg_subsample > djvu::max_subsample_ratio)
        throw Config::Error("The specified subsampling ratio is outside the allowed range");
      break;
    case OPT_FG_COLORS:
      this->fg_colors = parse_fg_colors(optarg);
      break;
    case OPT_MONOCHROME:
      this->monochrome = true;
      break;
    case OPT_LOSS_100:
      this->loss_level = 100;
      break;
    case OPT_LOSS_ANY:
      this->loss_level = string::as<int>(optarg);
      if (this->loss_level < 0)
        this->loss_level = 0;
      else if (this->loss_level > 200)
        this->loss_level = 200;
      break;
    case OPT_PAGES:
      parse_pages(optarg, this->pages);
      break;
    case OPT_ANTIALIAS:
      this->antialias = 1;
      break;
    case OPT_HYPERLINKS:
      parse_hyperlinks_options(optarg, this->hyperlinks);
      break;
    case OPT_NO_HLINKS:
      this->hyperlinks.extract = false;
      break;
    case OPT_NO_METADATA:
      this->extract_metadata = false;
      break;
    case OPT_VERBATIM_METADATA:
      this->adjust_metadata = false;
      break;
    case OPT_NO_OUTLINE:
      this->extract_outline = false;
      break;
    case OPT_NO_RENDER:
      this->no_render = 1;
      break;
    case OPT_TEXT_NONE:
      this->text = this->TEXT_NONE;
      break;
    case OPT_TEXT_WORDS:
      this->text = this->TEXT_WORDS;
      break;
    case OPT_TEXT_LINES:
      this->text = this->TEXT_LINES;
      break;
    case OPT_TEXT_NO_NFKC:
      this->text_nfkc = false;
      break;
    case OPT_OUTPUT:
      this->format = this->FORMAT_BUNDLED;
      this->output = optarg;
      this->output_stdout = false;
      break;
    case OPT_INDIRECT:
      this->format = this->FORMAT_INDIRECT;
      this->output = optarg;
      this->output_stdout = false;
      break;
    case OPT_PREFIX:
      this->pageid_prefix = parse_pageid_prefix(optarg);
      break;
    case OPT_HELP:
      throw NeedHelp();
    case OPT_VERSION:
      throw NeedVersion();
    case '?':
    case ':':
      throw InvalidOption();
    default:
      throw std::logic_error("Unknown option");
    }
  }
  if (optind < argc - 1)
    throw Config::Error("Too many arguments were specified");
  else if (optind > argc - 1)
    throw Config::Error("No input file name was specified");
  else
    this->file_name = argv[optind];
}

void Config::usage(const Config::Error &error)
{
  DebugStream &log = debug(0, this->verbose);
  if (error.is_already_printed())
    log << std::endl;
  if (!error.is_quiet())
    log << error << std::endl << std::endl;
  log
    << "Usage: " << std::endl
    << "   pdf2djvu [-o <output-djvu-file>] [options] <pdf-file>" << std::endl
    << "   pdf2djvu  -i <index-djvu-file>   [options] <pdf-file>" << std::endl
    << std::endl << "Options:"
    << std::endl << " -i, --indirect=FILE"
    << std::endl << " -o, --output=FILE"
    << std::endl << "     --pageid-prefix=NAME"
    << std::endl << " -d, --dpi=resolution"
    << std::endl << "     --media-box"
    << std::endl << "     --page-size=WxH"
    << std::endl << "     --bg-slices=N,...,N"
    << std::endl << "     --bg-slices=N+...+N"
    << std::endl << "     --bg-subsample=N"
#ifdef HAVE_GRAPHICSMAGICK
    << std::endl << "     --fg-colors=web"
    << std::endl << "     --fg-colors=N"
#endif
    << std::endl << "     --monochrome"
    << std::endl << "     --loss-level=N"
    << std::endl << "     --lossy"
    << std::endl << "     --anti-alias"
    << std::endl << "     --no-metadata"
    << std::endl << "     --verbatim-metadata"
    << std::endl << "     --no-outline"
    << std::endl << "     --hyperlinks=border-avis"
    << std::endl << "     --hyperlinks=#RRGGBB"
    << std::endl << "     --hyperlinks=none"
    << std::endl << "     --no-text"
    << std::endl << "     --words"
    << std::endl << "     --lines"
    << std::endl << "     --no-nfkc"
    << std::endl << " -p, --pages=..."
    << std::endl << " -v, --verbose"
    << std::endl << " -q, --quiet"
    << std::endl << " -h, --help"
    << std::endl << "     --version"
    << std::endl;
  exit(1);
}

// vim:ts=2 sw=2 et
