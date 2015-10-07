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

#ifndef PDF2DJVU_STRING_UTILS_HH
#define PDF2DJVU_STRING_UTILS_HH

#include <string>
#include <vector>

namespace string {

    void split(const std::string &s, char c, std::vector<std::string> &result);
    void replace_all(std::string& s, const std::string& pat, const std::string& repl);
    void replace_all(std::string& s, char pat, const std::string& repl);

}

#endif

// vim:ts=4 sts=4 sw=4 et
