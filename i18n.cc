/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include <clocale>

#include "i18n.hh"
#include "paths.hh"

#ifdef ENABLE_NLS

void i18n::setup_locale()
{
  std::setlocale(LC_ALL, "");
  /* Deliberately ignore errors. */
}

void i18n::setup()
{
  i18n::setup_locale();
  bindtextdomain(PACKAGE_NAME, paths::localedir);
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

// vim:ts=2 sw=2 et
