/* Copyright © 2007-2019 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_PDF_UNICODE_H
#define PDF2DJVU_PDF_UNICODE_H

#include <ostream>
#include <string>

#include <CharTypes.h>

#include "pdf-backend.hh"

namespace pdf
{

/* Unicode → UTF-8 conversion
 * ==========================
 */

    void write_as_utf8(std::ostream &stream, Unicode unicode_char);

    std::string string_as_utf8(const pdf::String *);
    std::string string_as_utf8(pdf::Object &);

/* class pdf::NFKC
 * ===============
 */

    class NFKC
    {
    public:
        virtual int length() const = 0;
        virtual operator const Unicode*() const = 0;
        virtual ~NFKC()
        { }
    };


/* class pdf::FullNFKC
 * ===================
 */

    class FullNFKC : public NFKC
    {
    protected:
        Unicode* data;
        int length_;
    public:
        explicit FullNFKC(const Unicode *, int length);
        ~FullNFKC();
        int length() const
        {
            return this->length_;
        }
        operator const Unicode*() const
        {
            return this->data;
        }
    };

/* class pdf::MinimalNFKC
 * ======================
 */

    class MinimalNFKC : public NFKC
    {
    protected:
        std::basic_string<Unicode> string;
    public:
        explicit MinimalNFKC(const Unicode *, int length);
        int length() const;
        operator const Unicode*() const;
    };
}

#endif

// vim:ts=4 sts=4 sw=4 et
