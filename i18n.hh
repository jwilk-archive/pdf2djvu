/* Copyright © 2009-2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_I18N
#define PDF2DJVU_I18N

#include "version.hh"

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
