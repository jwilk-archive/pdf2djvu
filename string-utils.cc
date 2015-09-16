/* Copyright © 2008-2015 Jakub Wilk <jwilk@jwilk.net>
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

#include "string-utils.hh"

#include <cstddef>

void string::split(const std::string &s, char c, std::vector<std::string> &result)
{
    size_t lpos = 0;
    while (true) {
        size_t rpos = s.find(c, lpos);
        result.push_back(s.substr(lpos, rpos - lpos));
        if (rpos == std::string::npos)
            break;
        else
            lpos = rpos + 1;
    }
}

// vim:ts=4 sts=4 sw=4 et
