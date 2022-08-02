/* Copyright © 2007-2022 Jakub Wilk <jwilk@jwilk.net>
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

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <sstream>

#include "autoconf.hh"

#include <CharTypes.h>
#include <PDFDocEncoding.h>
#include <UnicodeMapFuncs.h>
#include <UTF.h>
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

std::string pdf::string_as_utf8(const pdf::String *string)
{
    /* See
     * https://unicode.org/faq/utf_bom.html
     * for description of both UTF-16 and UTF-8.
     */
    const static uint32_t replacement_character = 0xFFFD;
    const char *cstring = pdf::get_c_string(string);
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
                for (size_t j = nbytes - 1; j; j--) {
                    buffer[j] = 0x80 | (code & 0x3F);
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

pdf::FullNFKC::FullNFKC(const Unicode *unistr, int length)
: data(nullptr), length_(0)
{
    assert(length >= 0);
    this->data = unicodeNormalizeNFKC(const_cast<Unicode *>(unistr), length, &this->length_, nullptr);
}

pdf::FullNFKC::~FullNFKC()
{
    gfree(this->data);
}

/* class pdf::MinimalNFKC
 * ======================
 */

pdf::MinimalNFKC::MinimalNFKC(const Unicode *unistr, int length)
{
    this->string.append(unistr, length);
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
