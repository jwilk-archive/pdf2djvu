/* Copyright © 2008-2015 Jakub Wilk <jwilk@jwilk.net>
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

void string::replace_all(std::string& s, const std::string& pat, const std::string& repl)
{
    size_t pos = 0;
    while ((pos = s.find(pat, pos)) != std::string::npos) {
         s.replace(pos, pat.length(), repl);
         pos += repl.length();
    }
}

void string::replace_all(std::string& s, char pat, const std::string& repl)
{
    replace_all(s, std::string(1, pat), repl);
}

// vim:ts=4 sts=4 sw=4 et
