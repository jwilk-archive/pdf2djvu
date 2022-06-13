/* Copyright © 2015-2022 Jakub Wilk <jwilk@jwilk.net>
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

template <typename ConstT, typename T>
class const_adapter
{
protected:
    ConstT x;
public:
    explicit const_adapter(ConstT x)
    : x(x)
    { }
    operator T () const
    {
        return const_cast<T>(x);
    }
    operator ConstT () const
    {
        return x;
    }
};

// vim:ts=4 sts=4 sw=4 et
