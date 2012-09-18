dnl | Copyright © 2008, 2009, 2010, 2011 Jakub Wilk
dnl | 
dnl | This package is free software; you can redistribute it and/or modify
dnl | it under the terms of the GNU General Public License as published by
dnl | the Free Software Foundation; version 2 dated June, 1991.

AC_PREREQ(2.61)
AC_INIT([pdf2djvu], [0.7.15], [http://code.google.com/p/pdf2djvu/issues/], [pdf2djvu], [http://pdf2djvu.googlecode.com/])
AC_CONFIG_AUX_DIR(tools)
AC_LANG(C++)

# Checks for programs.

AC_PROG_CXX
AC_PROG_INSTALL
AC_PREPROC_IFELSE(
  [AC_LANG_SOURCE([[#include <winver.h>]])],
  [AC_CHECK_TOOL(WINDRES, windres)]
)

# Checks for libraries.

PKG_CHECK_MODULES(DJVULIBRE, ddjvuapi >= 3.5.17)
djvulibre_bin_path=$($PKG_CONFIG --variable exec_prefix ddjvuapi)/bin
AC_SUBST(djvulibre_bindir, "$djvulibre_bin_path")
djvulibre_version=$($PKG_CONFIG --modversion ddjvuapi)
PACKAGE_STRING="$PACKAGE_STRING (DjVuLibre $djvulibre_version"
AC_DEFINE_UNQUOTED(DJVULIBRE_VERSION_STRING, "$djvulibre_version", Define to the version of DjVuLibre)

djvulibre_bugs=''
AC_MSG_CHECKING(DjVuLibre fitness)
for tool in cjb2 c44 csepdjvu ddjvu djvused djvuextract djvumake djvmcvt
do
  if test ! -x "$djvulibre_bin_path/$tool"
  then
    AC_MSG_RESULT(incomplete set of tools)
    AC_MSG_ERROR($tool not found)
  fi
done

# Test for Debian bug #458211 <http://bugs.debian/458211>.
# csepdjvu affected by the bug is unable to use subsample ratio 12.
# Prior to pdf2djvu 0.5.0, pdf2djvu simply dissalowed subsample ratio 12 by
# itself.
echo 'P1 12 12' > conftest.458211.pbm
yes 0 | head -n 144 >> conftest.458211.pbm
"$djvulibre_bin_path/cjb2" conftest.458211.pbm conftest.458211.djvu
"$djvulibre_bin_path/ddjvu" -format=rle conftest.458211.djvu conftest.458211.sep
printf 'P6 1 1 255 xxx' >> conftest.458211.sep
"$djvulibre_bin_path/csepdjvu" conftest.458211.sep conftest.458211.djvu 2>/dev/null || djvulibre_bugs="$djvulibre_bugs 458211"

# Test for Debian bug #471149 <http://bugs.debian.org/471149>.
# djvused affected by the bug is unable to correctly set annotations.
printf 'P1 3 3 0 0 0 0 0 0 0 0 0' > conftest.471149.pbm
"$djvulibre_bin_path/cjb2" conftest.471149.pbm conftest.471149.djvu
cat > conftest.471149.djvused <<_ACEOF
select 1
set-ant
(x)
.
_ACEOF
"$djvulibre_bin_path/djvused" -f conftest.471149.djvused -s conftest.471149.djvu
if ! "$djvulibre_bin_path/djvused" -e output-ant conftest.471149.djvu | grep '(x)' > /dev/null
then
  djvulibre_bugs="$djvulibre_bugs 471149"
fi

# Test for Debian bug #458086 <http://bugs.debian.org/458086>.
# djvused affected by the bug is unable to set bookmarks for indirect
# multi-page documents.
# Prior to pdf2djvu 0.5.3, the bug was worked around.
printf 'P1 3 3 0 0 0 0 0 0 0 0 0 0' > conftest.458086.pbm
"$djvulibre_bin_path/cjb2" conftest.458086.pbm conftest.458086.p1.djvu
"$djvulibre_bin_path/cjb2" conftest.458086.pbm conftest.458086.p2.djvu
printf 'AT&TFORM\0\0\0\065DJVMDIRM\0\0\0\051\001\0\002\377\377\306\277\212' > conftest.458086.djvu
printf '\037\353\113\377\100\216\067\206\077\154\276\170\002\074\223\233' >> conftest.458086.djvu
printf '\027\127\026\147\035\172\353\127\370\363\176\144\062\102\210\322\117' >> conftest.458086.djvu
printf '(bookmarks ("" ""))' > conftest.458086.outline
"$djvulibre_bin_path/djvused" -s -e 'set-outline conftest.458086.outline' conftest.458086.djvu
if ! "$djvulibre_bin_path/djvused" -e print-outline conftest.458086.djvu | grep '(bookmarks' > /dev/null
then
  djvulibre_bugs="$djvulibre_bugs 458086"
fi

if test -n "$djvulibre_bugs"
then
  result=''
  for bug in $djvulibre_bugs
  do
    if test -n "$result"
    then
      result="$result, "
    fi
    result="${result}<http://bugs.debian.org/$bug>"
  done
  AC_MSG_RESULT(buggy)
  AC_MSG_ERROR(See $result for details.)
else
  AC_MSG_RESULT(ok)
fi

PKG_CHECK_MODULES(POPPLER, poppler-splash >= 0.7.3)
poppler_version=$($PKG_CONFIG --modversion poppler-splash)
AC_DEFINE_UNQUOTED(POPPLER_VERSION_STRING, "$poppler_version", Define to the version of poppler)
PACKAGE_STRING="$PACKAGE_STRING, poppler $poppler_version"
poppler_version=$(printf '%s' "$poppler_version" | sed -e 's/\(^[[0-9]]\+[[.]][[0-9]]\+$\)/\1.0/; s/[[.]]/ /g')
poppler_version=$(printf '%02d' $poppler_version | sed -e 's/^0*//g')
AC_DEFINE_UNQUOTED(POPPLER_VERSION, $poppler_version, Define to the version of poppler, as integer)

AC_ARG_WITH(graphicsmagick, AS_HELP_STRING([--without-graphicsmagick], [do not use the GraphicsMagick library]))
if test "$with_graphicsmagick" != "no"
then
  PKG_CHECK_MODULES(GRAPHICSMAGICK, GraphicsMagick++,
    [
      AC_DEFINE(HAVE_GRAPHICSMAGICK, 1, Define if you have GraphicsMagick++ installed)
      graphicsmagick_version=$($PKG_CONFIG --modversion GraphicsMagick++)
      AC_DEFINE_UNQUOTED(GRAPHICSMAGICK_VERSION_STRING, "$graphicsmagick_version", Define to the version of GraphicsMagick++)
      PACKAGE_STRING="$PACKAGE_STRING, GraphicsMagick++ $graphicsmagick_version"
    ], [ : ]
  )
fi

AC_ARG_WITH(xslt, AS_HELP_STRING([--without-xslt], [do not use the GNOME XSLT library]))
if test "$with_xslt" != "no"
then
  PKG_CHECK_MODULES(XSLT, libxslt,
    [
      AC_DEFINE(HAVE_LIBXSLT, 1, Define if you have GNOME XSLT library installed)
      libxml2_version=$($PKG_CONFIG --modversion libxml-2.0)
      libxslt_version=$($PKG_CONFIG --modversion libxslt)
      PACKAGE_STRING="$PACKAGE_STRING, GNOME XSLT $libxslt_version, GNOME XML $libxml2_version"
    ], [ : ]
  )
fi

PACKAGE_STRING="$PACKAGE_STRING)"

AC_DEFINE_UNQUOTED(PACKAGE_STRING, "$PACKAGE_STRING", Define to the full name and version of this package)

AC_PREPROC_IFELSE([AC_LANG_SOURCE([[]])])

have_pstreams=maybe
AC_CHECK_FUNC(fork, 
  [AC_DEFINE(HAVE_FORK, 1, [Define if fork(3) is available.])],
  [have_pstreams=no]
)
AC_MSG_CHECKING(for PStreams)
if test "$have_pstreams" = "maybe"
then
  AC_PREPROC_IFELSE(
    [AC_LANG_SOURCE([[#include <pstream.h>]])],
    [
      AC_DEFINE(HAVE_PSTREAM_H, 1, [Define if you have the <pstream.h> file])
      have_pstreams=yes
    ]
  )
fi
if test "$have_pstreams" = "maybe"
then
  AC_PREPROC_IFELSE(
    [AC_LANG_SOURCE([[#include <pstreams/pstream.h>]])],
    [
      AC_DEFINE(HAVE_PSTREAMS_PSTREAM_H, 1, [Define if you have the <pstreams/pstream.h> file])
      have_pstreams=yes
    ]
  )
fi
if test "$have_pstreams" = "yes"
then
  AC_DEFINE(HAVE_PSTREAMS, 1, [Define if you have PStreams installed and usable])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no (or not usable))
fi

AM_GNU_GETTEXT(external, need-ngettext)
AM_GNU_GETTEXT_VERSION(0.17)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

AC_MSG_CHECKING(for wchar_t encoding)
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
    AC_MSG_RESULT(UTF-32 or UTF-16)
    have_unicode_wchar_t=yes
    AC_DEFINE(HAVE_UNICODE_WCHAR_T, 1, [Define if wchar_t encoding is UTF-32 or UTF-16])
  ],
  [
    AC_MSG_RESULT(unknown)
    have_unicode_wchar_t=maybe
  ]
)

AC_ARG_ENABLE(openmp, AS_HELP_STRING([--disable-openmp], [do not use the OpenMP API]))
if test "$enable_openmp" != "no"
then
  AX_OPENMP
  AC_SUBST(OPENMP_CXXFLAGS)
fi

AC_MSG_CHECKING(for MinGW ANSI stdio)
AC_EGREP_HEADER(__mingw_vsnprintf, stdio.h,
  [
    AC_MSG_RESULT(yes)
    AC_DEFINE_UNQUOTED(USE_MINGW_ANSI_STDIO, 1, Define to use MinGW ANSI stdio)
  ],
  [
    AC_MSG_RESULT(no)
  ]
)

# Turn on compile warnings

AX_CXXFLAGS_WARN_ALL(CXXFLAGS)
if test "$GXX" = yes
then
  CXXFLAGS="$CXXFLAGS -Werror=overloaded-virtual"
fi

# Output files

AC_CONFIG_FILES(Makefile.common)
AC_CONFIG_HEADER(version.hh)
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

if test "$with_graphicsmagick" != "no" && test -z "$graphicsmagick_version"
then
  cat <<_ACEOF

INFO: Color quantization using GraphicsMagick will be disabled.
Please consider installing the GraphicsMagick++ library to enable this
functionality.
_ACEOF
fi

if test "$with_xslt" != "no" && test -z "$libxslt_version"
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

# vim:ts=2 sw=2 et
