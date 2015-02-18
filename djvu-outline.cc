/* Copyright © 2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "djvu-outline.hh"

#include <cassert>
#include <limits>
#include <stdexcept>

#include "i18n.hh"

class OutlineError
: public std::runtime_error
{
public:
    OutlineError()
    : std::runtime_error(_("Document outline too large"))
    { }    
};

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
    for (std::vector<djvu::OutlineItem>::const_iterator it = this->children.begin(); it != this->children.end(); it++)
        size += it->size();
    return size;
}

size_t djvu::Outline::size() const
{
    size_t size = 0;
    for (std::vector<djvu::OutlineItem>::const_iterator it = this->items.begin(); it != this->items.end(); it++)
        size += it->size();
    return size;
}

template <int nbits>
static void print_int(std::ostream &stream, size_t value)
{
    assert(nbits % 8 == 0);
    assert(nbits <= std::numeric_limits<size_t>::digits);
    if (value >= (1 << nbits))
        throw OutlineError();
    for (int i = (nbits / 8) - 1; i >= 0; i--)
        stream << static_cast<char>((value >> (8 * i)) & 0xff);
}

std::ostream& djvu::operator<<(std::ostream &stream, const djvu::Outline &outline)
{
    print_int<16>(stream, outline.size());
    for (std::vector<djvu::OutlineItem>::const_iterator it = outline.items.begin(); it != outline.items.end(); it++)
        stream << *it;
    return stream;
}

djvu::Outline::operator bool() const
{
    return this->items.size() > 0;
}

std::ostream& djvu::operator<<(std::ostream &stream, const djvu::OutlineItem &item)
{
    print_int<8>(stream, item.children.size());
    print_int<24>(stream, item.description.size());
    stream << item.description;
    print_int<24>(stream, item.url.size());
    stream << item.url;
    for (std::vector<djvu::OutlineItem>::const_iterator it = item.children.begin(); it != item.children.end(); it++)
        stream << *it;
    return stream;
}

// vim:ts=4 sts=4 sw=4 et
