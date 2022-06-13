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

#ifndef PDF2DJVU_DJVU_OUTLINE_H
#define PDF2DJVU_DJVU_OUTLINE_H

#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace djvu
{

    class OutlineError
    : public std::runtime_error
    {
    public:
        OutlineError();
    };

    class OutlineItem;

    class OutlineBase
    {
    public:
        virtual OutlineItem& add(std::string description, std::string url) = 0;
        OutlineBase() = default;
        OutlineBase(const OutlineBase &) = default;
        virtual ~OutlineBase()
        { }
    };

    class OutlineItem
    : public OutlineBase
    {
    public:
        OutlineItem(const std::string &description, const std::string &url)
        : description(description),
          url(url)
        { }
        OutlineItem& add(std::string description, std::string url);
    private:
        std::vector<OutlineItem> children;
        std::string description;
        std::string url;
        size_t size() const;
        friend std::ostream &operator<<(std::ostream &, const OutlineItem &);
        friend class Outline;
    };

    class Outline
    : public OutlineBase
    {
    private:
        std::vector<OutlineItem> items;
        size_t size() const;
    public:
        OutlineItem& add(std::string description, std::string url);
        operator bool() const;
        friend std::ostream &operator<<(std::ostream &, const Outline &);
    };

    std::ostream &operator<<(std::ostream &, const OutlineItem &);
    std::ostream &operator<<(std::ostream &, const Outline &);

}

#endif

// vim:ts=4 sts=4 sw=4 et
