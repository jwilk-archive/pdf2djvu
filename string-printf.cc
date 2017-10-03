/* Copyright © 2007-2016 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
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

#include "string-printf.hh"

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include "autoconf.hh"
#include "system.hh"

#if USE_MINGW_ANSI_STDIO
// https://sourceforge.net/p/mingw-w64/wiki2/gnu%20printf/
#define vsnprintf __mingw_vsnprintf
#endif

std::string string_vprintf(const char *message, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int length = vsnprintf(nullptr, 0, message, args_copy);
    va_end(args_copy);
    if (length < 0)
        throw_posix_error("vsnprintf()");
    if (length == INT_MAX) {
        errno = ENOMEM;
        throw_posix_error("vsnprintf()");
    }
    std::vector<char> buffer(length + 1);
    length = vsprintf(buffer.data(), message, args);
    if (length < 0)
        throw_posix_error("vsprintf()");
    return buffer.data();
}

std::string string_printf(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    std::string result = string_vprintf(message, args);
    va_end(args);
    return result;
}

// vim:ts=4 sts=4 sw=4 et
