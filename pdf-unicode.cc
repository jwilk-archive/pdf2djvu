/* Copyright © 2007-2016 Jakub Wilk <jwilk@jwilk.net>
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

#include "pdf-unicode.hh"

#include <cstddef>
#include <cassert>
#include <climits>
#include <sstream>

#include <stdint.h>

#include "autoconf.hh"

#include <CharTypes.h>
#include <PDFDocEncoding.h>
#if POPPLER_VERSION >= 6200
#include <UnicodeMapFuncs.h>
#elif (POPPLER_VERSION < 2100) || (POPPLER_VERSION >= 2101)
#include <UTF8.h>
#endif
#if POPPLER_VERSION >= 2100
#include <UTF.h>
#endif
#include <UnicodeTypeTable.h>

#include <goo/gmem.h>

/* Unicode → UTF-8 conversion
 * ==========================
 */

void pdf::write_as_utf8(std::ostream &stream, Unicode unicode_char)
{
    char buffer[8];
    int seqlen = mapUTF8(unicode_char, buffer, sizeof buffer);
    stream.write(buffer, seqlen);
}

std::string pdf::string_as_utf8(pdf::String *string)
{
    /* See
     * https://unicode.org/faq/utf_bom.html
     * for description of both UTF-16 and UTF-8.
     */
    const static uint32_t replacement_character = 0xFFFD;
    const char *cstring = string->getCString();
    size_t clength = string->getLength();
    std::ostringstream stream;
    if (clength >= 2 && (cstring[0] & 0xFF) == 0xFE && (cstring[1] & 0xFF) == 0xFF) {
        /* UTF-16-BE Byte Order Mark */
        uint32_t code, code_shift = 0;
        for (size_t i = 2; i < clength; i += 2) {
            if (i + 1 < clength)
                code = ((cstring[i] & 0xFF) << 8) + (cstring[i + 1] & 0xFF);
            else {
                /* lone byte */
                code = replacement_character;
            }
            if (code_shift) {
                if (code >= 0xDC00 && code < 0xE000) {
                    /* trailing surrogate */
                    code = code_shift + (code & 0x3FF);
                    if (code >= 0x110000)
                        code = replacement_character;
                } else {
                    /* unpaired surrogate */
                    code = replacement_character;
                }
                code_shift = 0;
            } else if (code >= 0xD800 && code < 0xDC00) {
                /* leading surrogate */
                code_shift = 0x10000 + ((code & 0x3FF) << 10);
                continue;
            }
            if ((code & 0xFFFE) == 0xFFFE) {
                /* non-character */
                code = replacement_character;
            }
            if (code < 0x80) {
                char ascii = code;
                stream << ascii;
            } else {
                char buffer[4];
                size_t nbytes;
                for (nbytes = 2; nbytes < 4; nbytes++)
                    if (code < (1U << (5 * nbytes + 1)))
                        break;
                buffer[0] = (0xFF00 >> nbytes) & 0xFF;
                for (size_t i = nbytes - 1; i; i--) {
                    buffer[i] = 0x80 | (code & 0x3F);
                    code >>= 6;
                }
                buffer[0] |= code;
                stream.write(buffer, nbytes);
            }
        }
    } else {
        /* PDFDoc encoding */
        for (size_t i = 0; i < clength; i++)
            write_as_utf8(stream, pdfDocEncoding[cstring[i] & 0xFF]);
    }
    return stream.str();
}

std::string pdf::string_as_utf8(pdf::Object &object)
{
    return pdf::string_as_utf8(object.getString());
}

/* class pdf::FullNFKC
 * ===================
 */

pdf::FullNFKC::FullNFKC(Unicode *unistr, int length)
: data(nullptr), length_(0)
{
    assert(length >= 0);
    this->data = unicodeNormalizeNFKC(unistr, length, &this->length_, nullptr);
}

pdf::FullNFKC::~FullNFKC()
{
    gfree(this->data);
}

/* class pdf::MinimalNFKC
 * ======================
 */

#if POPPLER_VERSION < 1900
const Unicode apf_min = 0xFB00;
const Unicode apf_max = 0xFB4F;
#endif

pdf::MinimalNFKC::MinimalNFKC(Unicode *unistr, int length)
{
    /* Poppler 0.19.0 (and later versions) performs NFKC normalization of
     * characters from the Alphabetic Presentation Forms block
     * (U+FB00–U+FB4F):
     * https://bugs.freedesktop.org/show_bug.cgi?id=7002
     * For older versions, we do the same normalization ourselves.
     */
#if POPPLER_VERSION < 1900
    std::basic_ostringstream<Unicode> stream;
    for (int i = 0; i < length; i++) {
        if (unistr[i] >= apf_min && unistr[i] <= apf_max) {
            int clen;
            Unicode *cstr = unicodeNormalizeNFKC(unistr + i, 1, &clen, nullptr);
            stream.write(cstr, clen);
            gfree(cstr);
        } else {
            stream.write(unistr + i, 1);
        }
    }
    this->string += stream.str();
#else
    this->string.append(unistr, length);
#endif
}

int pdf::MinimalNFKC::length() const
{
    assert(this->string.length() <= INT_MAX);
    return this->string.length();
}

pdf::MinimalNFKC::operator const Unicode*() const
{
    return this->string.c_str();
}

// vim:ts=4 sts=4 sw=4 et
