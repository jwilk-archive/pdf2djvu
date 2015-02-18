/* Copyright © 2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#ifndef PDF2DJVU_DJVU_OUTLINE_H
#define PDF2DJVU_DJVU_OUTLINE_H

#include <ostream>
#include <string>
#include <vector>

namespace djvu
{

    class OutlineItem;

    class OutlineBase
    {
    public:
        virtual OutlineItem& add(std::string description, std::string url) = 0;
    };

    class OutlineItem
    : public OutlineBase
    {
    public:
        OutlineItem(std::string description, std::string url)
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

};

#endif

// vim:ts=4 sts=4 sw=4 et
