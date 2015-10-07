/* Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
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

#include "autoconf.hh"

#if !HAVE_TIMEGM

#include "sys-time.hh"

#include <cerrno>

time_t timegm(struct tm *tm)
{
    time_t y = tm->tm_year + 1900;
    if (y < 1970) {
        errno = ERANGE;
        return -1;
    }
    time_t n = (
        y * 365 + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400 - 719527 +
        tm->tm_yday
    ) * 24 * 60 * 60;
    n += (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec;
    return n;
}

#endif
