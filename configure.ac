dnl | Copyright © 2008-2015 Jakub Wilk <jwilk@jwilk.net>
dnl |
dnl | This package is free software; you can redistribute it and/or modify
dnl | it under the terms of the GNU General Public License as published by
dnl | the Free Software Foundation; version 2 dated June, 1991.

AC_PREREQ(2.62)
AC_INIT([pdf2djvu], [0.8.1], [https://bitbucket.org/jwilk/pdf2djvu/issues], [pdf2djvu], [http://jwilk.net/software/pdf2djvu])
AC_CONFIG_AUX_DIR([tools])
AC_LANG([C++])

# Checks for programs.

AC_PROG_CXX
AC_PROG_INSTALL
AC_PREPROC_IFELSE(
  [AC_LANG_SOURCE([[#include <winver.h>]])],
  [AC_CHECK_TOOL([WINDRES], [windres])]
)

# Checks for libraries.

PKG_CHECK_MODULES([DJVULIBRE], [ddjvuapi >= 3.5.21])
djvulibre_bin_path=$($PKG_CONFIG --variable exec_prefix ddjvuapi)/bin
AC_SUBST([djvulibre_bindir], ["$djvulibre_bin_path"])
djvulibre_version=$($PKG_CONFIG --modversion ddjvuapi)
AC_DEFINE_UNQUOTED([DJVULIBRE_VERSION_STRING], ["$djvulibre_version"], [Define to the version of DjVuLibre])

AC_MSG_CHECKING(DjVuLibre fitness)
for tool in bzz c44 cjb2 csepdjvu djvuextract djvumake djvused djvmcvt
do
  if ! test -x "$djvulibre_bin_path/$tool$EXEEXT"
  then
    AC_MSG_RESULT([incomplete set of tools])
    AC_MSG_ERROR([$tool$EXEEXT not found])
  fi
done
AC_MSG_RESULT([ok])

PKG_CHECK_MODULES([POPPLER], [poppler-splash >= 0.11.1])
poppler_version=$($PKG_CONFIG --modversion poppler-splash)
AC_DEFINE_UNQUOTED([POPPLER_VERSION_STRING], ["$poppler_version"], [Define to the version of poppler])
poppler_version=$(printf '%s' "$poppler_version" | sed -e 's/\(^[[0-9]]\+[[.]][[0-9]]\+$\)/\1.0/; s/[[.]]/ /g')
poppler_version=$(printf '%02d' $poppler_version | sed -e 's/^0*//g')
AC_DEFINE_UNQUOTED([POPPLER_VERSION], [$poppler_version], [Define to the version of poppler, as integer])
AC_MSG_CHECKING([for Poppler xpdf headers])
original_cppflags="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $POPPLER_CFLAGS"
AC_PREPROC_IFELSE(
  [AC_LANG_SOURCE([[#include <GfxState.h>]])],
  [AC_MSG_RESULT([yes])],
  [
    AC_MSG_RESULT([no])
    AC_MSG_FAILURE([missing Poppler xpdf headers])
  ]
)
CPPFLAGS="$original_cppflags"

AC_ARG_WITH([graphicsmagick], [AS_HELP_STRING([--without-graphicsmagick], [do not use the GraphicsMagick library])])
if test "$with_graphicsmagick" != "no"
then
  PKG_CHECK_MODULES([GRAPHICSMAGICK], [GraphicsMagick++],
    [
      AC_DEFINE([HAVE_GRAPHICSMAGICK], 1, [Define if you have GraphicsMagick++ installed])
      have_graphicsmagick=yes
    ], [ : ]
  )
fi

AC_ARG_WITH([xslt], [AS_HELP_STRING([--without-xslt], [do not use the GNOME XSLT library])])
if test "$with_xslt" != "no"
then
  PKG_CHECK_MODULES([XSLT], [libxslt],
    [
      AC_DEFINE([HAVE_LIBXSLT], 1, [Define if you have GNOME XSLT library installed])
      have_xslt=yes
    ], [ : ]
  )
fi

AC_ARG_WITH([pstreams], [AS_HELP_STRING([--without-pstreams], [do not use the PStreams library])])
have_pstreams=maybe
AC_CHECK_FUNC([fork],
  [AC_DEFINE([HAVE_FORK], 1, [Define if fork(3) is available.])],
  [have_pstreams=no]
)
if test "$with_pstreams" != "no"
then
  AC_MSG_CHECKING([for PStreams])
  if test "$have_pstreams" = "maybe"
  then
    AC_PREPROC_IFELSE(
      [AC_LANG_SOURCE([[#include <pstream.h>]])],
      [
        AC_DEFINE([HAVE_PSTREAM_H], 1, [Define if you have the <pstream.h> file])
        have_pstreams=yes
      ]
    )
  fi
  if test "$have_pstreams" = "maybe"
  then
    AC_PREPROC_IFELSE(
      [AC_LANG_SOURCE([[#include <pstreams/pstream.h>]])],
      [
        AC_DEFINE([HAVE_PSTREAMS_PSTREAM_H], 1, [Define if you have the <pstreams/pstream.h> file])
        have_pstreams=yes
      ]
    )
  fi
  if test "$have_pstreams" = "yes"
  then
    AC_DEFINE([HAVE_PSTREAMS], 1, [Define if you have PStreams installed and usable])
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no (or not usable)])
  fi
fi

AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.17)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

AC_MSG_CHECKING([for wchar_t encoding])
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>

#if defined(__STDC_ISO_10646__)
#elif defined(__WIN32__)
#elif defined(__APPLE__)
#else
#error
#endif
]])],
  [
    AC_MSG_RESULT([UTF-32 or UTF-16])
    have_unicode_wchar_t=yes
    AC_DEFINE([HAVE_UNICODE_WCHAR_T], 1, [Define if wchar_t encoding is UTF-32 or UTF-16])
  ],
  [
    AC_MSG_RESULT([unknown])
    have_unicode_wchar_t=maybe
  ]
)

AC_OPENMP

AC_MSG_CHECKING([for MinGW ANSI stdio])
AC_EGREP_HEADER([__mingw_vsnprintf], [stdio.h],
  [
    AC_MSG_RESULT([yes])
    AC_DEFINE_UNQUOTED([USE_MINGW_ANSI_STDIO], 1, [Define to use MinGW ANSI stdio])
  ],
  [
    AC_MSG_RESULT([no])
  ]
)

# Turn on compile warnings

AX_CXXFLAGS_WARN_ALL([CXXFLAGS])
if test "$GXX" = yes
then
  CXXFLAGS="$CXXFLAGS -Werror=overloaded-virtual -Werror=narrowing"
fi

# Output files

AC_CONFIG_FILES([autoconf.mk])
AC_CONFIG_HEADER([autoconf.hh])
AC_OUTPUT

# Final remarks

if test "$ax_cv_cxx_openmp" = "unknown"
then
  cat <<_ACEOF

WARNING: Parallel page rendering will be disabled.
Please consider using a compiler with OpenMP support to enable this
functionality.
_ACEOF
fi

if test "$with_graphicsmagick" != "no" && test -z "$have_graphicsmagick"
then
  cat <<_ACEOF

INFO: Color quantization using GraphicsMagick will be disabled.
Please consider installing the GraphicsMagick++ library to enable this
functionality.
_ACEOF
fi

if test "$with_xslt" != "no" && test -z "$have_xslt"
then
  cat <<_ACEOF

WARNING: Updating XMP metadata will be disabled.
Please consider installing the GNOME XSLT library to enable this
functionality.
_ACEOF
fi

if test "$have_unicode_wchar_t" = maybe
then
  cat <<_ACEOF

WARNING: pdf2djvu assumes that wchar_t encoding is UTF-32 or UTF-16.
This might NOT be the case for your system.
Please file a bug to $PACKAGE_BUGREPORT.
_ACEOF
fi

# vim:ts=2 sts=2 sw=2 et
