/* Copyright © 2018-2020 Jakub Wilk <jwilk@jwilk.net>
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

#ifndef PDF2DJVU_DUMMY_MUTEX
#define PDF2DJVU_DUMMY_MUTEX

#include <mutex>

#if !defined(_GLIBCXX_HAS_GTHREADS)

namespace std {

    class recursive_mutex {
    public:
        void lock() { }
        void unlock() { }
        bool try_lock() {
            return true;
        }
    };

    class mutex {
    public:
        void lock() { }
        void unlock() { }
        bool try_lock() {
            return true;
        }
    };

}

#endif

#endif

// vim:ts=4 sts=4 sw=4 et
