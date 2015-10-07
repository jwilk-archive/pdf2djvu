/* Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_I18N
#define PDF2DJVU_I18N

#include "autoconf.hh"

namespace i18n
{

  void setup_locale();
  void setup();
}

static inline void N_(const char *message_id)
{
  return;
}

#ifdef ENABLE_NLS

#include <libintl.h>

static inline const char * _(const char *message_id)
{
  return gettext(message_id);
}

#else

static inline const char * ngettext(const char *message_id, const char *message_id_plural, unsigned long int n)
{
  return n == 1 ? message_id : message_id_plural;
}

static inline const char * _(const char *message_id)
{
  return message_id;
}

#endif

#endif

// vim:ts=2 sts=2 sw=2 et
