/* Copyright © 2009-2022 Jakub Wilk <jwilk@jwilk.net>
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

#include "system.hh"

#include <cstddef>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

#include <errno.h>
#include <iconv.h>

#if WIN32
#include <windows.h>
#endif

#include "const-adapter.hh"

namespace encoding {

#if WIN32

template <>
std::ostream &operator <<(std::ostream &stream, const proxy<native, terminal> &converter)
{
    const std::string &string = converter.string;
    unsigned int handle_id = 0;
    HANDLE handle = nullptr;
    if (&stream == &std::cout)
        handle_id = STD_OUTPUT_HANDLE;
    else if (&stream == &std::cerr || &stream == &std::clog)
        handle_id = STD_ERROR_HANDLE;
    if (handle_id) {
        handle = GetStdHandle(handle_id);
        if (handle == INVALID_HANDLE_VALUE)
            throw_win32_error("GetStdHandle()");
    }
    if (handle != nullptr) {
        unsigned long mode;
        bool ok = GetConsoleMode(handle, &mode);
        if (!ok)
            handle = nullptr;
    }
    if (handle == nullptr) {
        stream << string;
        return stream;
    }
    size_t wide_length;
    unsigned long written_length;
    size_t length = converter.string.length();
    if (length == 0)
        return stream;
    stream.flush();
    std::vector<wchar_t> wide_buffer(length);
    wide_length = MultiByteToWideChar(
        CP_ACP, 0,
        string.c_str(), length,
        wide_buffer.data(), length
    );
    if (wide_length == 0)
        throw_win32_error("MultiByteToWideChar()");
    bool ok = WriteConsoleW(handle, wide_buffer.data(), wide_length, &written_length, nullptr);
    if (!ok)
        throw_win32_error("WriteConsoleW()");
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
    explicit IConv(const char *tocode, const char *fromcode="")
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
        // POSIX requires that “inbuf” type must be “char **”.
        // But on MacOS X, it is “const char **”.
        // This adapter automatically converts “const char **”
        // to whichever type is needed.
        typedef const_adapter<const char **, char **> const_adapter;
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
