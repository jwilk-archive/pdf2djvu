/* Copyright © 2007-2016 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_STRING_PRINTF_HH
#define PDF2DJVU_STRING_PRINTF_HH

#include <cstdarg>
#include <string>

std::string string_vprintf(const char *message, va_list args);
#if defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
std::string string_printf(const char *message, ...);

#endif

// vim:ts=4 sts=4 sw=4 et
