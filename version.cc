/* Copyright © 2015 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "autoconf.hh"

#include <sstream>

#include "system.hh"
#include "version.hh"

#if HAVE_PSTREAMS
static std::string get_pstreams_version()
{
    return string_printf("%d.%d.%d",
        PSTREAMS_VERSION >> 8,
        (PSTREAMS_VERSION >> 4) & 0xF,
        PSTREAMS_VERSION & 0xF
    );
}
#endif

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
#if HAVE_PSTREAMS
    stream << ", PStreams " << get_pstreams_version();
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
#if HAVE_PSTREAMS
    stream << "+ PStreams " << get_pstreams_version() << "\n";
#endif
    return stream.str();
}

// vim:ts=4 sts=4 sw=4 et
