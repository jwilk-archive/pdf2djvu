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

template <typename tp>
class Array
{
private:
    Array(const Array &); // not defined
    Array& operator=(const Array &); // not defined
protected:
    tp *buffer;
public:
    explicit Array(size_t size)
    {
        this->buffer = new tp[size];
    }
    operator tp * ()
    {
        return this->buffer;
    }
    ~Array() throw ()
    {
        delete[] this->buffer;
    }
};

// vim:ts=4 sts=4 sw=4 et
