/* Copyright © 2009-2022 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_XMP_H
#define PDF2DJVU_XMP_H

#include <string>
#include <stdexcept>

#include "pdf-backend.hh"

namespace xmp
{
    std::string transform(const std::string &data, const pdf::Metadata &metadata);

    class Error : public std::runtime_error
    {
    public:
        explicit Error(const std::string &message)
        : std::runtime_error(message)
        { }
    };
}

#endif

// vim:ts=4 sts=4 sw=4 et
