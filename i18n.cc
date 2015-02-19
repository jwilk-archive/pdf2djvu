/* Copyright © 2009-2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <clocale>

#include "i18n.hh"
#include "paths.hh"
#include "system.hh"

#if ENABLE_NLS

void i18n::setup_locale()
{
  std::setlocale(LC_ALL, "");
  /* Deliberately ignore errors. */
}

void i18n::setup()
{
#if WIN32
  std::string basedir(program_dir);
#else
  std::string basedir("/");
#endif
  std::string localedir = absolute_path(paths::localedir, basedir);
  i18n::setup_locale();
  bindtextdomain(PACKAGE_NAME, localedir.c_str());
  /* Deliberately ignore errors. */
  textdomain(PACKAGE_NAME);
  /* Deliberately ignore errors. */
}

#else

void i18n::setup_locale()
{
  std::setlocale(LC_CTYPE, "");
  /* Deliberately ignore errors. */
}

void i18n::setup()
{
  i18n::setup_locale();
}

#endif

// vim:ts=2 sts=2 sw=2 et
