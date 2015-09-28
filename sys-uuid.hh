/* Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdfdjvu.
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

#ifndef PDF2DJVU_SYS_UUID_H
#define PDF2DJVU_SYS_UUID_H

#include "autoconf.hh"

#if WIN32

#include <rpc.h>

void uuid_generate_random(uuid_t &uu);
void uuid_unparse_lower(uuid_t &uu, char *out);

#elif HAVE_DCE_UUID

#include <uuid.h>

void uuid_generate_random(uuid_t &uu);
void uuid_unparse_lower(uuid_t &uu, char *out);

#elif HAVE_LIBUUID

#include <uuid.h>

#endif

#endif

// vim:ts=4 sts=4 sw=4 et
