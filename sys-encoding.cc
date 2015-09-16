/* Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
 *
 * This file is part of pdfdjvu.
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

#include "system.hh"

#include <cstddef>
#include <cstdio>
#include <ostream>
#include <string>

#include <errno.h>
#include <iconv.h>

#if WIN32
#include <windows.h>
#endif

#include "array.hh"

namespace encoding {

#if WIN32

// The native encoding (so called ANSI character set) can differ from the
// terminal encoding (typically: so called OEM charset).
template <>
std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
{
    const std::string &string = converter.string;
    size_t wide_length, new_length, length = converter.string.length();
    if (length == 0)
        return stream;
    Array<char> buffer(length * 2);
    Array<wchar_t> wide_buffer(length);
    wide_length = MultiByteToWideChar(
        CP_ACP, 0,
        string.c_str(), length,
        wide_buffer, length
    );
    if (wide_length == 0)
        throw_win32_error("MultiByteToWideChar");
    new_length = WideCharToMultiByte(
        GetConsoleOutputCP(), 0,
        wide_buffer, wide_length,
        buffer, length * 2,
        NULL, NULL
    );
    if (new_length == 0)
        throw_win32_error("WideCharToMultiByte");
    stream.write(buffer, new_length);
    return stream;
}

#else

template <>
std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
{
    stream << converter.string;
    return stream;
}

#endif

class IConv
{
protected:
    iconv_t cd;
public:
    IConv(const char *tocode, const char *fromcode="")
    {
        this->cd = iconv_open(tocode, fromcode);
        if (this->cd == reinterpret_cast<void*>(-1))
            throw_posix_error("iconv_open()");
    }
    ~IConv()
    {
        int rc = iconv_close(this->cd);
        if (rc < 0)
            throw_posix_error("iconv_close()");
    }
    size_t operator()(const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
    {
        // POSIX requires that “inbuf” type is “char **”.
        // But on MacOS X, it is “const char **”.
        // This adapter automatically converts “const char **”
        // to whichever type is needed.
        class const_adapter
        {
        protected:
            const char **s;
        public:
            const_adapter(const char **s)
            : s(s)
            { }
            operator char ** () const
            {
                return const_cast<char **>(s);
            }
            operator const char** () const
            {
                return s;
            }
        };
        return iconv(this->cd, const_adapter(inbuf), inbytesleft, outbuf, outbytesleft);
    }
};

template <>
std::ostream &operator <<(std::ostream &stream, const proxy<native, utf8> &converter)
{
    IConv iconv("UTF-8");
    char outbuf[BUFSIZ];
    char *outbuf_ptr = outbuf;
    size_t outbuf_len = sizeof outbuf;
    const char *inbuf = converter.string.c_str();
    size_t inbuf_len = converter.string.length();
    while (inbuf_len > 0) {
        size_t n = iconv(&inbuf, &inbuf_len, &outbuf_ptr, &outbuf_len);
        if (n == static_cast<size_t>(-1)) {
            if (errno == E2BIG) {
                stream.write(outbuf, outbuf_ptr - outbuf);
                outbuf_ptr = outbuf;
                outbuf_len = sizeof outbuf;
            } else
                throw Error();
        } else if (n > 0) {
            errno = EILSEQ;
            throw Error();
        }
    }
    stream.write(outbuf, outbuf_ptr - outbuf);
    return stream;
}

}

// vim:ts=4 sts=4 sw=4 et
