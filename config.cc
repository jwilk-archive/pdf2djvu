/* Copyright © 2007, 2008, 2009, 2010 Jakub Wilk
 * Copyright © 2009 Mateusz Turcza
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
#include "i18n.hh"
#include "version.hh"

class OptionsParseError : public Config::Error
{
protected:
  explicit OptionsParseError(const std::string &message)
  : Config::Error(message)
  { }
public:
  OptionsParseError()
  : Config::Error(_("Unable to parse command-line options"))
  { }
};

class TooManyArguments : public Config::Error
{
public:
  TooManyArguments()
  : Config::Error(_("Too many arguments were specified"))
  { }
};

class NoInputFile : public Config::Error
{
public:
  NoInputFile()
  : Config::Error(_("No input file name was specified"))
  { }
};

class InvalidOutputFileName : public Config::Error
{
public:
  InvalidOutputFileName()
  : Config::Error(_("Invalid output file name"))
  { }
};

class PagesParseError : public Config::Error
{
public:
  PagesParseError()
  : Config::Error(_("Unable to parse page numbers"))
  { }
};

class PageSizeParseError : public Config::Error
{
public:
  PageSizeParseError()
  : Config::Error(_("Unable to parse page size"))
  { }
};

class HyperlinksOptionsParseError : public Config::Error
{
public:
  HyperlinksOptionsParseError()
  : Config::Error(_("Unable to parse hyperlinks options"))
  { }
};

class DpiOutsideRange : public Config::Error
{
public:
  DpiOutsideRange(int dpi_from, int dpi_to)
  : Config::Error(string_printf(_("The specified resolution is outside the allowed range: %d .. %d"), dpi_from, dpi_to))
  { }
};

class FgColorsOutsideRange : public Config::Error
{
public:
  FgColorsOutsideRange(unsigned int n, unsigned int m)
  : Error(string_printf(_("The specified number of foreground colors is outside the allowed range: %u .. %u"), n, m))
  { }
};

class SubsampleRatioOutsideRange : public Config::Error
{
public:
  SubsampleRatioOutsideRange(unsigned int n, unsigned int m)
  : Error(string_printf(_("The specified subsampling ratio is outside the allowed range: %u .. %u"), n, m))
  { }
};

class PageTitleTemplateParseError : public Config::Error
{
public:
  PageTitleTemplateParseError()
  : Config::Error(_("Unable to parse page title template specification"))
  { }
};

class PageidTemplateParseError : public Config::Error
{
public:
  PageidTemplateParseError()
  : Config::Error(_("Unable to parse pageid template specification"))
  { }
};

class PageidIllegalCharacter : public Config::Error
{
public:
  PageidIllegalCharacter()
  : Config::Error(_("Pageid must consist only of letters, digits, '_', '+', '-' and '.' characters"))
  { }
};

class PageidIllegalDot : public Config::Error
{
public:
  PageidIllegalDot()
  : Config::Error(_("Pageid cannot start with a '.' character or contain two consecutive '.' characters"))
  { }
};

class PageidBadExtension : public Config::Error
{
public:
  PageidBadExtension()
  : Config::Error(_("Pageid must end with the '.djvu' or the '.djv' extension"))
  { }
};

string_format::Template* Config::default_pageid_template(const std::string &prefix)
{
  return new string_format::Template(prefix + "{spage:04*}.djvu");
}

Config::Config()
{
  this->text = this->TEXT_WORDS;
  this->text_nfkc = true;
  this->text_crop = false;
  this->format = this->FORMAT_BUNDLED;
  this->output_stdout = true;
  this->verbose = 1;
  this->dpi = 300;
  this->guess_dpi = false;
  this->preferred_page_size = std::make_pair(0, 0);
  this->use_media_box = false;
  this->bg_subsample = 3;
  this->fg_colors = this->FG_COLORS_DEFAULT;
  this->antialias = false;
  this->extract_metadata = true;
  this->adjust_metadata = true;
  this->extract_outline = true;
  this->no_render = false;
  this->monochrome = false;
  this->loss_level = 0;
  this->bg_slices = NULL;
  this->file_name = NULL;
  this->pageid_template.reset(default_pageid_template("p"));
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
    throw HyperlinksOptionsParseError();
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
        throw PagesParseError();
      result.push_back(std::make_pair(value[0], value[1]));
      value[0] = value[1] = 0;
      state = 0;
    }
    else
      throw PagesParseError();
  }
  if (state == 0)
    value[1] = value[0];
  if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
    throw PagesParseError();
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
    throw PageSizeParseError();
}

class InvalidNumber : public Config::Error
{
public:
  explicit InvalidNumber(const char *s)
  : Config::Error(string_printf(_("%s is not a valid number"), s))
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

static unsigned int parse_fg_colors(const std::string &s)
{
  if (s == "web")
    return Config::FG_COLORS_WEB;
  else if (s == "default")
    return Config::FG_COLORS_DEFAULT;
  long n = string::as<long>(s);
  if (n < 1 || n > static_cast<long>(djvu::max_fg_colors))
    throw FgColorsOutsideRange(1, djvu::max_fg_colors);
  return n;
}

static unsigned int parse_bg_subsample(const std::string &s)
{
  long n = string::as<long>(optarg);
  if (n < 1 || n > static_cast<long>(djvu::max_subsample_ratio))
    throw SubsampleRatioOutsideRange(1, djvu::max_subsample_ratio);
  return n;
}

static void validate_pageid_template(string_format::Template &pageid_template)
{
  string_format::Bindings empty_bindings;
  std::string pageid = pageid_template.format(empty_bindings);
  size_t length = pageid.length();
  bool dot_allowed = false;
  for (std::string::const_iterator it = pageid.begin(); it != pageid.end(); it++)
  {
    if (*it == '.')
    {
      if (!dot_allowed)
        throw PageidIllegalDot();
      dot_allowed = false;
    }
    else if ((*it >= 'a' && *it <= 'z') || (*it >= '0' && *it <= '9') || *it == '_' || *it == '-' || *it == '+')
      dot_allowed = true;
    else
      throw PageidIllegalCharacter();
  }
  if (length >= 4 && pageid.substr(length - 4) == ".djv")
    ;
  else if (length >= 5 && pageid.substr(length - 5) == ".djvu")
    ;
  else
    throw PageidBadExtension();
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
    OPT_GUESS_DPI,
    OPT_HYPERLINKS,
    OPT_LOSS_100,
    OPT_LOSS_ANY,
    OPT_MEDIA_BOX,
    OPT_MONOCHROME,
    OPT_NO_HLINKS,
    OPT_NO_METADATA,
    OPT_NO_OUTLINE,
    OPT_NO_RENDER,
    OPT_PAGEID_PREFIX,
    OPT_PAGEID_TEMPLATE,
    OPT_PAGE_SIZE,
    OPT_PAGE_TITLE_TEMPLATE,
    OPT_TEXT_CROP,
    OPT_TEXT_FILTER,
    OPT_TEXT_LINES,
    OPT_TEXT_NONE,
    OPT_TEXT_NO_NFKC,
    OPT_TEXT_WORDS,
    OPT_VERBATIM_METADATA,
    OPT_VERSION,
  };
  static struct option options [] =
  {
    { "anti-alias", 0, 0, OPT_ANTIALIAS },
    { "antialias", 0, 0, OPT_ANTIALIAS },
    { "bg-slices", 1, 0, OPT_BG_SLICES },
    { "bg-subsample", 1, 0, OPT_BG_SUBSAMPLE },
    { "crop-text", 0, 0, OPT_TEXT_CROP },
    { "dpi", 1, 0, OPT_DPI },
    { "fg-colors", 1, 0, OPT_FG_COLORS },
    { "filter-text", 1, 0, OPT_TEXT_FILTER },
    { "guess-dpi", 0, 0, OPT_GUESS_DPI },
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
    { "no-nfkc", 0, 0, OPT_TEXT_NO_NFKC },
    { "no-outline", 0, 0, OPT_NO_OUTLINE },
    { "no-render", 0, 0, OPT_NO_RENDER },
    { "no-text", 0, 0, OPT_TEXT_NONE },
    { "output", 1, 0, OPT_OUTPUT },
    { "page-size", 1, 0, OPT_PAGE_SIZE },
    { "page-title-template", 1, 0, OPT_PAGE_TITLE_TEMPLATE },
    { "pageid-prefix", 1, 0, OPT_PAGEID_PREFIX },
    { "pageid-template", 1, 0, OPT_PAGEID_TEMPLATE },
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
      throw OptionsParseError();
    switch (c)
    {
    case OPT_DPI:
      this->dpi = string::as<int>(optarg);
      if (this->dpi < djvu::min_dpi || this->dpi > djvu::max_dpi)
        throw DpiOutsideRange(djvu::min_dpi, djvu::max_dpi);
      break;
    case OPT_GUESS_DPI:
      this->guess_dpi = true;
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
      this->bg_subsample = parse_bg_subsample(optarg);
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
      this->no_render = true;
      this->monochrome = true;
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
    case OPT_TEXT_FILTER:
      this->text_nfkc = false; /* filter normally does some normalization on its own */
      this->text_filter_command_line = optarg;
      break;
    case OPT_TEXT_CROP:
      this->text_crop = true;
      break;
    case OPT_OUTPUT:
      this->format = this->FORMAT_BUNDLED;
      this->output = optarg;
      if (this->output == "-")
      {
        this->output.clear();
        this->output_stdout = true;
      }
      else
      {
        std::string directory_name, file_name;
        this->output_stdout = false;
        split_path(this->output, directory_name, file_name);
        if (file_name == "-")
        {
          /* ``djvmcvt`` does not support ``-`` as a file name.
           * Work-arounds are possible but not worthwhile.
           */
          throw InvalidOutputFileName();
        }
      }
      break;
    case OPT_INDIRECT:
      this->format = this->FORMAT_INDIRECT;
      this->output = optarg;
      this->output_stdout = false;
      break;
    case OPT_PAGEID_PREFIX:
      this->pageid_template.reset(this->default_pageid_template(optarg));
      validate_pageid_template(*this->pageid_template);
      break;
    case OPT_PAGEID_TEMPLATE:
      try
      {
        this->pageid_template.reset(new string_format::Template(optarg));
      }
      catch (string_format::ParseError)
      {
        throw PageidTemplateParseError();
      }
      validate_pageid_template(*this->pageid_template);
      break;
    case OPT_PAGE_TITLE_TEMPLATE:
      try
      {
        this->page_title_template.reset(new string_format::Template(optarg));
      }
      catch (string_format::ParseError)
      {
        throw PageTitleTemplateParseError();
      }
      break;
    case OPT_HELP:
      throw NeedHelp();
    case OPT_VERSION:
      throw NeedVersion();
    case '?':
    case ':':
      throw InvalidOption();
    default:
      throw std::logic_error(_("Unknown option"));
    }
  }
  if (optind < argc - 1)
    throw TooManyArguments();
  else if (optind > argc - 1)
    throw NoInputFile();
  else
    this->file_name = argv[optind];
}

void Config::usage(const Config::Error &error) const
{
  DebugStream &log = debug(0, this->verbose);
  if (error.is_already_printed())
    log << std::endl;
  if (!error.is_quiet())
    log << error << std::endl << std::endl;
  log
    << _("Usage: ") << std::endl
    << _("   pdf2djvu [-o <output-djvu-file>] [options] <pdf-file>") << std::endl
    << _("   pdf2djvu  -i <index-djvu-file>   [options] <pdf-file>") << std::endl
    << std::endl << _("Options: ")
    << std::endl << _(" -i, --indirect=FILE")
    << std::endl << _(" -o, --output=FILE")
    << std::endl << _("     --pageid-prefix=NAME")
    << std::endl << _("     --pageid-template=TEMPLATE")
    << std::endl << _("     --page-title-template=TEMPLATE")
    << std::endl << _(" -d, --dpi=RESOLUTION")
    << std::endl <<   "     --guess-dpi"
    << std::endl <<   "     --media-box"
    << std::endl << _("     --page-size=WxH")
    << std::endl <<   "     --bg-slices=N,...,N"
    << std::endl <<   "     --bg-slices=N+...+N"
    << std::endl <<   "     --bg-subsample=N"
    << std::endl <<   "     --fg-colors=default"
    << std::endl <<   "     --fg-colors=web"
#ifdef HAVE_GRAPHICSMAGICK
    << std::endl <<   "     --fg-colors=N"
#endif
    << std::endl <<   "     --monochrome"
    << std::endl <<   "     --loss-level=N"
    << std::endl <<   "     --lossy"
    << std::endl <<   "     --anti-alias"
    << std::endl <<   "     --no-metadata"
    << std::endl <<   "     --verbatim-metadata"
    << std::endl <<   "     --no-outline"
    << std::endl <<   "     --hyperlinks=border-avis"
    << std::endl <<   "     --hyperlinks=#RRGGBB"
    << std::endl <<   "     --no-hyperlinks"
    << std::endl <<   "     --no-text"
    << std::endl <<   "     --words"
    << std::endl <<   "     --lines"
    << std::endl <<   "     --crop-text"
    << std::endl <<   "     --no-nfkc"
    << std::endl << _("     --filter-text=COMMAND-LINE")
    << std::endl <<   " -p, --pages=..."
    << std::endl <<   " -v, --verbose"
    << std::endl <<   " -q, --quiet"
    << std::endl <<   " -h, --help"
    << std::endl <<   "     --version"
    << std::endl;
}

// vim:ts=2 sw=2 et
