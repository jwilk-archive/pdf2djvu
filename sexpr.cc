/* Copyright © 2008-2018 Jakub Wilk <jwilk@jwilk.net>
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

#include "sexpr.hh"

#include <cstdio>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

static int my_puts(miniexp_io_t* io, const char *s)
{
    std::ostream *puts_stream = reinterpret_cast<std::ostream*>(io->data[0]);
    *puts_stream << s;
    return puts_stream->good() ? 0 : EOF;
}

namespace sexpr
{
    std::ostream &operator<<(std::ostream &stream, const sexpr::Ref &expr)
    {
        miniexp_io_t io;
        miniexp_io_init(&io);
        io.fputs = my_puts;
        io.data[0] = &stream;
        miniexp_prin_r(&io, expr);
        return stream;
    }
}

#if _OPENMP && DDJVUAPI_VERSION < 23

#include <omp.h>

class OmpLock
{
private:
    omp_lock_t m_lock;
public:
    OmpLock()
    {
        omp_init_lock(&this->m_lock);
    }
    void lock()
    {
        omp_set_lock(&this->m_lock);
    }
    void unlock()
    {
        omp_unset_lock(&this->m_lock);
    }
    ~OmpLock()
    {
        omp_destroy_lock(&this->m_lock);
    }
};

static OmpLock omp_lock;

sexpr::Guard::Guard()
{
    omp_lock.lock();
}

sexpr::Guard::~Guard()
{
    omp_lock.unlock();
}

#else

sexpr::Guard::Guard() { }
sexpr::Guard::~Guard() { }

#endif

// vim:ts=4 sts=4 sw=4 et
