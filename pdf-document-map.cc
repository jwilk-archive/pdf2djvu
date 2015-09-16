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
    for (std::vector<const char*>::const_iterator it = paths.begin(); it != paths.end(); it++)
    {
        this->indices.push_back(global_index);
        {
            std::ifstream ifs(*it, std::ifstream::in | std::ifstream::binary);
            ifs.seekg(0, std::ios::end);
            this->byte_size += ifs.tellg();
            ifs.close();
        }
        {
            std::auto_ptr<pdf::Document> doc(new pdf::Document(*it));
            pdf::Catalog *catalog = doc->getCatalog();
            for (int i = 0; i < doc->getNumPages(); i++) {
                pdf::String s;
                if (catalog->indexToLabel(i, &s)) {
                    std::string str = pdf::string_as_utf8(&s);
#if POPPLER_VERSION < 1903
                    // Prior to 0.19.3, Poppler incorrectly adds trailing
                    // NULL character to Unicode labels:
                    // http://cgit.freedesktop.org/poppler/poppler/commit/?id=cef6ac0ebbf8
                    // Let's strip it.
                    if (str.length() > 0 && str.end()[-1] == '\0')
                        str.erase(str.end() - 1);
#endif
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
