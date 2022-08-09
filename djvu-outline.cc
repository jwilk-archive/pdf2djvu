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

#include "djvu-outline.hh"

#include <cassert>
#include <limits>
#include <stdexcept>

#include "i18n.hh"

djvu::OutlineError::OutlineError()
: std::runtime_error(_("Document outline too large"))
{ }

djvu::OutlineItem& djvu::OutlineItem::add(std::string description, std::string url)
{
    djvu::OutlineItem child(description, url);
    this->children.push_back(child);
    return this->children.back();
}

djvu::OutlineItem& djvu::Outline::add(std::string description, std::string url)
{
    djvu::OutlineItem item(description, url);
    this->items.push_back(item);
    return this->items.back();
}

size_t djvu::OutlineItem::size() const
{
    size_t size = 1;
    for (const djvu::OutlineItem &child : this->children)
        size += child.size();
    return size;
}

size_t djvu::Outline::size() const
{
    size_t size = 0;
    for (const djvu::OutlineItem &item : this->items)
        size += item.size();
    return size;
}

template <int nbits>
static void print_int(std::ostream &stream, size_t value)
{
    assert(nbits % 8 == 0);
    assert(nbits <= std::numeric_limits<size_t>::digits);
    if (value >= static_cast<size_t>(1) << nbits)
        throw djvu::OutlineError();
    for (int i = (nbits / 8) - 1; i >= 0; i--)
        stream << static_cast<char>((value >> (8 * i)) & 0xFF);
}

template <int nbits>
static void print_int_le(std::ostream &stream, size_t value)
{
    assert(nbits % 8 == 0);
    assert(nbits <= std::numeric_limits<size_t>::digits);
    if (value >= static_cast<size_t>(1) << nbits)
        throw djvu::OutlineError();
    for (int i = 0; i < (nbits / 8); i++)
        stream << static_cast<char>((value >> (8 * i)) & 0xFF);
}

std::ostream& djvu::operator<<(std::ostream &stream, const djvu::Outline &outline)
{
    print_int<16>(stream, outline.size());
    for (const djvu::OutlineItem &item : outline.items)
        stream << item;
    return stream;
}

djvu::Outline::operator bool() const
{
    return this->items.size() > 0;
}

std::ostream& djvu::operator<<(std::ostream &stream, const djvu::OutlineItem &item)
{
    // https://sourceforge.net/p/djvu/bugs/346/
    // DjVu Reference (§8.3.3) says that each bookmark starts with:
    // • BYTE nChildren — number of immediate child bookmark records
    // • INT24 nDesc — size of description text
    // What's actually implemented in DjVuLibre is:
    // • INT16 (little-endian) nChildren
    // • INT16 (big-endian) nDesc
    print_int_le<16>(stream, item.children.size());
    print_int<16>(stream, item.description.size());
    stream << item.description;
    print_int<24>(stream, item.url.size());
    stream << item.url;
    for (const djvu::OutlineItem &child : item.children)
        stream << child;
    return stream;
}

// vim:ts=4 sts=4 sw=4 et
