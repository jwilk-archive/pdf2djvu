/* Copyright © 2007-2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "pdf-unicode.hh"

#include <sstream>

#include <PDFDocEncoding.h>
#if (POPPLER_VERSION < 2100) || (POPPLER_VERSION >= 2101)
#include <UTF8.h>
#endif
#if POPPLER_VERSION >= 2100
#include <UTF.h>
#endif
#include <UnicodeTypeTable.h>

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
     * http://unicode.org/faq/utf_bom.html
     * for description of both UTF-16 and UTF-8.
     */
    const static uint32_t replacement_character = 0xfffd;
    const char *cstring = string->getCString();
    size_t clength = string->getLength();
    std::ostringstream stream;
    if (clength >= 2 && (cstring[0] & 0xff) == 0xfe && (cstring[1] & 0xff) == 0xff) {
        /* UTF-16-BE Byte Order Mark */
        uint32_t code, code_shift = 0;
        for (size_t i = 2; i < clength; i += 2) {
            if (i + 1 < clength)
                code = ((cstring[i] & 0xff) << 8) + (cstring[i + 1] & 0xff);
            else {
                /* lone byte */
                code = replacement_character;
            }
            if (code_shift) {
                if (code >= 0xdc00 && code < 0xe000) {
                    /* trailing surrogate */
                    code = code_shift + (code & 0x3ff);
                    if (code >= 0x110000)
                        code = replacement_character;
                } else {
                    /* unpaired surrogate */
                    code = replacement_character;
                }
                code_shift = 0;
            } else if (code >= 0xd800 && code < 0xdc00) {
                /* leading surrogate */
                code_shift = 0x10000 + ((code & 0x3ff) << 10);
                continue;
            }
            if ((code & 0xfffe) == 0xfffe) {
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
                buffer[0] = (0xff00 >> nbytes) & 0xff;
                for (size_t i = nbytes - 1; i; i--) {
                    buffer[i] = 0x80 | (code & 0x3f);
                    code >>= 6;
                }
                buffer[0] |= code;
                stream.write(buffer, nbytes);
            }
        }
    } else {
        /* PDFDoc encoding */
        for (size_t i = 0; i < clength; i++)
            write_as_utf8(stream, pdfDocEncoding[cstring[i] & 0xff]);
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
: data(NULL), length_(0)
{
    assert(length >= 0);
    data = unicodeNormalizeNFKC(unistr, length, &this->length_, NULL);
}

pdf::FullNFKC::~FullNFKC() throw ()
{
    gfree(this->data);
}

#if POPPLER_VERSION >= 1900
/* Preempt this poppler function, so that it doesn't stand in our way. */
pdf::Bool unicodeIsAlphabeticPresentationForm(Unicode c)
{
    return 0;
}
#endif

// vim:ts=4 sts=4 sw=4 et
