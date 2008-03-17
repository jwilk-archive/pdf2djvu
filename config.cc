/* Copyright © 2007, 2008 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <stdexcept>

#include "config.hh"
#include "djvuconst.hh"

#include <getopt.h>

config::text_t config::text = config::TEXT_WORDS;
config::format_t config::format = config::FORMAT_BUNDLED;
std::string config::output;
bool config::output_stdout = true;
int config::verbose = 1;
int config::dpi = 300;
std::pair<int, int> config::preferred_page_size = std::make_pair(0, 0);
int config::bg_subsample = 3;
int config::fg_colors = -1;
bool config::antialias = false;
std::vector<sexpr::Ref> config::hyperlinks_options;
bool config::hyperlinks_user_border_color = false;
bool config::extract_hyperlinks = true;
bool config::extract_metadata = true;
bool config::extract_outline = true;
bool config::no_render = false;
char *config::bg_slices = NULL;
std::vector< std::pair<int, int> > config::pages;
char *config::file_name = NULL;
static void split_by_char(char c, const std::string &s, std::vector<std::string> &result)
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

static void parse_hyperlinks_options(const std::string &s, std::vector<sexpr::Ref> &result, bool &user_border_color)
{
  std::vector<std::string> splitted;
  split_by_char(',', s, splitted);
  for (std::vector<std::string>::const_iterator it = splitted.begin(); it != splitted.end(); it++)
  {
    if (*it == "border-avis" || *it == "border_avis")
    {
      sexpr::Ref expr = sexpr::cons(sexpr::symbol("border_avis"), sexpr::nil);
      result.push_back(expr);
      continue;
    }
    else if 
    (
      it->length() == 7 &&
      (*it)[0] == '#' &&
      it->find_first_not_of("0123456789abcdefABCDEF", 1) == std::string::npos
    )
    {
      sexpr::Ref expr = sexpr::cons(sexpr::symbol(it->c_str()), sexpr::nil);
      expr = sexpr::cons(sexpr::symbol("border"), expr);
      result.push_back(expr);
      user_border_color = true;
      continue;
    }
    throw config::HyperlinksOptionsParseError();
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
        throw config::PagesParseError();
      result.push_back(std::make_pair(value[0], value[1]));
      value[0] = value[1] = 0;
      state = 0;
    }
    else
      throw config::PagesParseError();
  }
  if (state == 0)
    value[1] = value[0];
  if (value[0] < 1 || value[1] < 1 || value[0] > value[1])
    throw config::PagesParseError();
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
    throw config::PageSizeParseError();
}

static int parse_fg_colors(const std::string &s)
{
  if (s == "web")
    return -1;
  int n = atoi(s.c_str());
  if (n < 1 || n > 4080)
    throw config::Error("The specified number of foreground colors is outside the allowed range");
  return n;
}

void config::read_config(int argc, char * const argv[])
{
  enum
  {
    OPT_ANTIALIAS    = 0x300,
    OPT_BG_SLICES    = 0x200,
    OPT_BG_SUBSAMPLE = 0x201,
    OPT_FG_COLORS    = 0x202,
    OPT_DPI          = 'd',
    OPT_HELP         = 'h',
    OPT_HYPERLINKS   = 0x501,
    OPT_INDIRECT     = 'i',
    OPT_NO_HLINKS    = 0x401,
    OPT_NO_METADATA  = 0x402,
    OPT_NO_OUTLINE   = 0x403,
    OPT_NO_RENDER    = 0x400,
    OPT_OUTPUT       = 'o',
    OPT_PAGES        = 'p',
    OPT_PAGE_SIZE    = 0x700,
    OPT_QUIET        = 'q',
    OPT_TEXT_LINES   = 0x102,
    OPT_TEXT_NONE    = 0x100,
    OPT_TEXT_WORDS   = 0x101,
    OPT_VERBOSE      = 'v',
    OPT_VERSION      = 0x600
  };
  static struct option options [] =
  {
    { "dpi",            1, 0, OPT_DPI },
    { "page-size",      1, 0, OPT_PAGE_SIZE },
    { "quiet",          0, 0, OPT_QUIET },
    { "verbose",        0, 0, OPT_VERBOSE },
    { "bg-slices",      1, 0, OPT_BG_SLICES },
    { "bg-subsample",   1, 0, OPT_BG_SUBSAMPLE },
    { "fg-colors",      1, 0, OPT_FG_COLORS },
    { "antialias",      0, 0, OPT_ANTIALIAS },
    { "hyperlinks",     1, 0, OPT_HYPERLINKS },
    { "no-hyperlinks",  0, 0, OPT_NO_HLINKS },
    { "no-metadata",    0, 0, OPT_NO_METADATA },
    { "no-outline",     0, 0, OPT_NO_OUTLINE },
    { "no-render",      0, 0, OPT_NO_RENDER },
    { "pages",          1, 0, OPT_PAGES },
    { "help",           0, 0, OPT_HELP },
    { "version",        0, 0, OPT_VERSION },
    { "no-text",        0, 0, OPT_TEXT_NONE },
    { "words",          0, 0, OPT_TEXT_WORDS },
    { "lines",          0, 0, OPT_TEXT_LINES },
    { "output",         1, 0, OPT_OUTPUT },
    { "indirect",       0, 0, OPT_INDIRECT },
    { NULL,             0, 0, '\0' }
  };
  int optindex, c;
  while (true)
  {
    optindex = 0;
    c = getopt_long(argc, argv, "i:o:d:qvp:h", options, &optindex);
    if (c < 0)
      break;
    if (c == 0)
      throw Error("");
    switch (c)
    {
    case OPT_DPI:
      config::dpi = atoi(optarg);
      if (config::dpi < djvu::MIN_DPI || config::dpi > djvu::MAX_DPI)
        throw config::DpiOutsideRange(djvu::MIN_DPI, djvu::MAX_DPI);
      break;
    case OPT_PAGE_SIZE:
      config::preferred_page_size = parse_page_size(optarg);
      break;
    case OPT_QUIET:
      config::verbose = 0;
      break;
    case OPT_VERBOSE:
      config::verbose++;
      break;
    case OPT_BG_SLICES:
      config::bg_slices = optarg;
      break;
    case OPT_BG_SUBSAMPLE:
      config::bg_subsample = atoi(optarg);
      if (config::bg_subsample < 1 || config::bg_subsample > 11)
        throw config::Error("The specified subsampling ratio is outside the allowed range");
      break;
    case OPT_FG_COLORS:
      config::fg_colors = parse_fg_colors(optarg);
      break;
    case OPT_PAGES:
      parse_pages(optarg, config::pages);
      break;
    case OPT_ANTIALIAS:
      config::antialias = 1;
      break;
    case OPT_HYPERLINKS:
      parse_hyperlinks_options(optarg, config::hyperlinks_options, config::hyperlinks_user_border_color);
      break;
    case OPT_NO_HLINKS:
      config::extract_hyperlinks = false;
      break;
    case OPT_NO_METADATA:
      config::extract_metadata = false;
      break;
    case OPT_NO_OUTLINE:
      config::extract_outline = false;
      break;
    case OPT_NO_RENDER:
      config::no_render = 1;
      break;
    case OPT_TEXT_NONE:
      config::text = config::TEXT_NONE;
      break;
    case OPT_TEXT_WORDS:
      config::text = config::TEXT_WORDS;
      break;
    case OPT_TEXT_LINES:
      config::text = config::TEXT_LINES;
      break;
    case OPT_OUTPUT:
      config::format = config::FORMAT_BUNDLED;
      config::output = optarg;
      config::output_stdout = false;
      break;
    case OPT_INDIRECT:
      config::format = config::FORMAT_INDIRECT;
      config::output = optarg;
      config::output_stdout = false;
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
    throw config::Error("Too many arguments were specified");
  else if (optind > argc - 1)
    throw config::Error("No input file name was specified");
  else
    config::file_name = argv[optind];
}

void config::usage(const config::Error &error)
{
  if (error.is_already_printed())
    debug(0) << std::endl;
  if (!error.is_quiet())
    debug(0) << error << std::endl << std::endl;
  debug(0) 
    << "Usage: " << std::endl
    << "   pdf2djvu [-o <output-djvu-file>] [options] <pdf-file>" << std::endl
    << "   pdf2djvu  -i <output-directory>  [options] <pdf-file>" << std::endl
    << std::endl
    << "Options:" << std::endl
    << " -i, --indirect=DIR"      << std::endl
    << " -o, --output=FILE"       << std::endl
    << " -v, --verbose"           << std::endl
    << " -q, --quiet"             << std::endl
    << " -d, --dpi=resolution"    << std::endl
    << "     --page-size=WxH"     << std::endl
    << "     --bg-slices=N,...,N" << std::endl
    << "     --bg-slices=N+...+N" << std::endl
    << "     --bg-subsample=N"    << std::endl
    << "     --fg-colors=web"     << std::endl
    << "     --fg-colors=N"       << std::endl
    << "     --antialias"         << std::endl
    << "     --no-metadata"       << std::endl
    << "     --no-outline"        << std::endl
    << "     --hyperlinks=..."    << std::endl
    << "     --no-hyperlinks"     << std::endl
    << "     --no-text"           << std::endl
    << "     --words"             << std::endl
    << "     --lines"             << std::endl
    << " -p, --pages=..."         << std::endl
    << " -h, --help"              << std::endl
    << "     --version"           << std::endl
  ;
  exit(1);
}

// vim:ts=2 sw=2 et
