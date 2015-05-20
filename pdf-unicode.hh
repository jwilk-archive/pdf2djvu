/* Copyright © 2007-2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_PDF_UNICODE_H
#define PDF2DJVU_PDF_UNICODE_H

#include <ostream>

#include "autoconf.hh"

#include <CharTypes.h>

#include "pdf-backend.hh"

namespace pdf
{

/* Unicode → UTF-8 conversion
 * ==========================
 */

    void write_as_utf8(std::ostream &stream, Unicode unicode_char);

    std::string string_as_utf8(pdf::String *);
    std::string string_as_utf8(pdf::Object &);

/* class pdf::NFKC
 * ===============
 */

    class NFKC
    {
    public:
        virtual int length() const = 0;
        virtual operator const Unicode*() const = 0;
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
        explicit FullNFKC(Unicode *, int length);
        ~FullNFKC() throw ();
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
        explicit MinimalNFKC(Unicode *, int length);
        int length() const;
        operator const Unicode*() const;
    };
}

#endif

// vim:ts=4 sts=4 sw=4 et
