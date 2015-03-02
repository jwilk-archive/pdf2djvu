/* Copyright © 2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "autoconf.hh"

#include <sstream>

#include "version.hh"

const std::string get_version()
{
    std::ostringstream stream;
    stream << PACKAGE_STRING;
    stream << " (DjVuLibre " DJVULIBRE_VERSION_STRING;
    stream << ", Poppler " POPPLER_VERSION_STRING;
#if HAVE_GRAPHICSMAGICK
    stream << ", GraphicsMagick++ " GRAPHICSMAGICK_VERSION_STRING;
#endif
#if HAVE_LIBXSLT
    stream << ", GNOME XSLT " LIBXSLT_VERSION_STRING;
    stream << ", GNOME XML " LIBXML2_VERSION_STRING;
#endif
    stream << ")";
    return stream.str();
}

const std::string get_multiline_version()
{
    std::ostringstream stream;
    stream << PACKAGE_STRING << "\n";
    stream << "+ DjVuLibre " DJVULIBRE_VERSION_STRING << "\n";
    stream << "+ Poppler " POPPLER_VERSION_STRING << "\n";
#if HAVE_GRAPHICSMAGICK
    stream << "+ GraphicsMagick++ " GRAPHICSMAGICK_VERSION_STRING << "\n";
#endif
#if HAVE_LIBXSLT
    stream << "+ GNOME XSLT " LIBXSLT_VERSION_STRING << "\n";
    stream << "+ GNOME XML " LIBXML2_VERSION_STRING << "\n";
#endif
    return stream.str();
}

// vim:ts=4 sts=4 sw=4 et
