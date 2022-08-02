dnl | Copyright © 2008-2022 Jakub Wilk <jwilk@jwilk.net>
dnl |
dnl | This file is part of pdf2djvu.
dnl |
dnl | pdf2djvu is free software; you can redistribute it and/or modify
dnl | it under the terms of the GNU General Public License version 2 as
dnl | published by the Free Software Foundation.
dnl |
dnl | pdf2djvu is distributed in the hope that it will be useful, but
dnl | WITHOUT ANY WARRANTY; without even the implied warranty of
dnl | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl | General Public License for more details.

AC_PREREQ(2.62)
AC_INIT([pdf2djvu], [0.9.19], [https://github.com/jwilk/pdf2djvu/issues], [pdf2djvu], [https://jwilk.net/software/pdf2djvu])
AC_CONFIG_AUX_DIR([tools])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG([C++])

# Checks for programs:

AC_PROG_EGREP
AC_PROG_INSTALL
AC_PREPROC_IFELSE(
  [AC_LANG_SOURCE([[#include <winver.h>]])],
  [AC_CHECK_TOOL([WINDRES], [windres])],
  [WINDRES=]
)

# Checks for the compiler:

AC_PROG_CXX

P_CXX11

AC_SYS_LARGEFILE

AC_OPENMP

AC_MSG_CHECKING([for MinGW ANSI stdio])
AC_EGREP_HEADER([__mingw_vsnprintf], [stdio.h],
  [
    # https://sourceforge.net/p/mingw-w64/wiki2/gnu%20printf/
    AC_MSG_RESULT([yes])
    AC_DEFINE_UNQUOTED([USE_MINGW_ANSI_STDIO], 1, [Define to use MinGW ANSI stdio])
  ],
  [
    AC_MSG_RESULT([no])
  ]
)

# Checks for libraries:

m4_pattern_forbid(
    [^PKG_CHECK_MODULES],
    [undefined macro PKG_CHECK_MODULES; install pkg-config and run private/autogen]
)

PKG_CHECK_MODULES([DJVULIBRE], [ddjvuapi >= 3.5.25])
djvulibre_bin_path=$($PKG_CONFIG --variable exec_prefix ddjvuapi)/bin
AC_SUBST([djvulibre_bindir], ["$djvulibre_bin_path"])
djvulibre_version=$($PKG_CONFIG --modversion ddjvuapi)
AC_DEFINE_UNQUOTED([DJVULIBRE_VERSION_STRING], ["$djvulibre_version"], [Define to the version of DjVuLibre])

AC_MSG_CHECKING([DjVuLibre fitness])
for tool in bzz c44 cjb2 csepdjvu djvuextract djvumake djvused djvmcvt
do
  if ! test -x "$djvulibre_bin_path/$tool$EXEEXT"
  then
    AC_MSG_RESULT([incomplete set of tools])
    AC_MSG_ERROR([$tool$EXEEXT not found])
  fi
done
AC_MSG_RESULT([ok])

PKG_CHECK_MODULES([POPPLER], [poppler >= 0.62.0])
poppler_version=$($PKG_CONFIG --modversion poppler)
AC_DEFINE_UNQUOTED([POPPLER_VERSION_STRING], ["$poppler_version"], [Define to the version of Poppler])
parse_poppler_version()
{
  test $# -eq 3 || return 1
  printf '%s' "${1#0}"
  shift
  for v in "$@"
  do
    case $v in
    ?) printf '0%s' "$v";;
    ??) printf '%s' "$v";;
    *) return 1;
    esac
  done
}
AS_IF(
  [poppler_version=$(IFS=.; parse_poppler_version $poppler_version)],
  [:],
  [AC_MSG_ERROR([could not parse Poppler version])]
)
AC_DEFINE_UNQUOTED([POPPLER_VERSION], [$poppler_version], [Define to the version of Poppler, as integer])
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

AC_ARG_ENABLE([xmp], [AS_HELP_STRING([--disable-xmp], [do not update XMP metadata])])
if test "$enable_xmp" != "no"
then
  PKG_CHECK_MODULES([EXIV2], [exiv2 >= 0.21], [have_exiv2=yes], [:])
  if test "$have_exiv2"
  then
    AC_MSG_CHECKING([for Windows RPC API])
    AC_PREPROC_IFELSE(
      [AC_LANG_SOURCE([[
        #if WIN32
        #include <rpc.h>
        #else
        #error
        #endif
      ]])], [
        AC_MSG_RESULT([yes])
        winuuid_libs='-lrpcrt4'
        have_uuid=yes
      ], [
        AC_MSG_RESULT([no])
      ]
    )
    if test ! "$have_uuid"
    then
      P_CHECK_FUNC(
        [
          #include <stdint.h>
          #include <uuid.h>
        ],
        [void], [uuid_create], [uuid_t *, uint32_t *],
        [
          AC_DEFINE([HAVE_DCE_UUID], 1, [Define if you have the DCE uuid_*() functions])
          have_uuid=yes
        ]
      )
    fi
    if test ! "$have_uuid"
    then
      PKG_CHECK_MODULES(
        [LIBUUID], [uuid],
        [
          have_uuid=yes
          AC_DEFINE([HAVE_LIBUUID], 1, [Define if you have the libuuid library installed])
        ], [:]
      )
    fi
    if test "$have_uuid"
    then
      AC_DEFINE([HAVE_XMP], 1, [Define if you have the Exiv2 and UUID libraries installed])
      have_xmp=yes
      AC_SUBST([XMP_CFLAGS], ["$EXIV2_CFLAGS $LIBUUID_CFLAGS"])
      AC_SUBST([XMP_LIBS], ["$EXIV2_LIBS $LIBUUID_LIBS $winuuid_libs"])
    fi
  fi
fi

AC_ARG_ENABLE([heap-profiling], [AS_HELP_STRING([--enable-heap-profiling], [enable heap profiling (using gperftools)])])
if test "$enable_heap_profiling" = 'yes'
then
  AC_SUBST([TCMALLOC_LIBS], ['-ltcmalloc'])
  AC_DEFINE([USE_HEAP_PROFILING], 1, [Define to enable heap profiling (using gperftools)])
fi

AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.17)
AM_ICONV

# Checks for library functions:

P_CHECK_FUNC(
  [#include <time.h>],
  [time_t], [timegm], [struct tm *],
)

# Turn on compile warnings:

P_MAYBE_ADD_CXXFLAGS(
  [-Wall],
  [-Wempty-body],
  [-Wswitch-enum],
  [-Werror=misleading-indentation],
  [-Werror=narrowing],
  [-Werror=overloaded-virtual],
)

# Output files:

AC_CONFIG_FILES([autoconf.mk])
AC_CONFIG_HEADERS([autoconf.hh])
AC_OUTPUT

# Final remarks:

if test "$ax_cv_cxx_openmp" = "unknown"
then
  cat <<_ACEOF

WARNING: Parallel page rendering will be disabled.
Please consider using a compiler with OpenMP support to enable this
feature.
_ACEOF
fi

if test "$with_graphicsmagick" != "no" && test -z "$have_graphicsmagick"
then
  cat <<_ACEOF

INFO: Color quantization using GraphicsMagick will be disabled.
Please consider installing the GraphicsMagick++ library to enable this
feature.
_ACEOF
fi

if test "$enable_xmp" != "no" && test -z "$have_xmp"
then
  cat <<_ACEOF

WARNING: Updating XMP metadata will be disabled.
Please consider installing the Exiv2 and libuuid libraries to enable this
feature.
_ACEOF
fi

# vim:ts=2 sts=2 sw=2 et
