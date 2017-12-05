/* Copyright © 2015-2016 Jakub Wilk <jwilk@jwilk.net>
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

#include "pdf-document-map.hh"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <memory>

#include "autoconf.hh"
#include "pdf-backend.hh"
#include "pdf-unicode.hh"

pdf::DocumentMap::DocumentMap(const std::vector<const char *> &paths)
: paths(paths)
{
    int global_index = 0;
    this->byte_size = 0;
    for (const char *path : paths)
    {
        this->indices.push_back(global_index);
        {
            std::ifstream ifs(path, std::ifstream::in | std::ifstream::binary);
            ifs.seekg(0, std::ios::end);
            this->byte_size += ifs.tellg();
            ifs.close();
        }
        {
            std::unique_ptr<pdf::Document> doc(new pdf::Document(path));
            pdf::Catalog *catalog = doc->getCatalog();
            for (int i = 0; i < doc->getNumPages(); i++) {
                pdf::String s;
                if (catalog->indexToLabel(i, &s)) {
                    std::string str = pdf::string_as_utf8(&s);
                    this->labels.push_back(str);
                }
                else
                    this->labels.push_back("");
                global_index++;
            }
        }
    }
    this->indices.push_back(global_index);
}

pdf::PageInfo pdf::DocumentMap::get(int global_pageno)
{
    int global_index = global_pageno - 1;
    size_t doc_index = std::upper_bound(
        this->indices.begin(),
        this->indices.end(),
        global_index
    ) - this->indices.begin() - 1;
    return pdf::PageInfo(
        global_index + 1,
        /* path = */ this->paths.at(doc_index),
        /* local_index = */ global_pageno - this->indices.at(doc_index),
        /* label = */ this->labels.at(global_index)
    );
}

// vim:ts=4 sts=4 sw=4 et
