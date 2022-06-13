/* Copyright © 2015-2022 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_PDF_DOCUMENT_MAP_HH
#define PDF2DJVU_PDF_DOCUMENT_MAP_HH

#include <cstdint>
#include <string>
#include <vector>

namespace pdf
{

    class PageInfo
    {
    public:
        const int global_pageno;
        const char *path;
        const int local_pageno;
        const std::string &label;
        PageInfo(int global_pageno, const char *path, int local_pageno, const std::string &label)
        : global_pageno(global_pageno),
          path(path),
          local_pageno(local_pageno),
          label(label)
        { }
    };

    class DocumentMap
    {
    protected:
        intmax_t byte_size;
        const std::vector<const char *> &paths;
        std::vector<std::string> labels;
        std::vector<int> indices;
    public:
        explicit DocumentMap(const std::vector<const char *> &paths);
        intmax_t get_byte_size()
        {
            return this->byte_size;
        }
        int get_n_pages()
        {
            return this->indices.back();
        }
        PageInfo get(int global_index);
    };

}

#endif

// vim:ts=4 sts=4 sw=4 et
