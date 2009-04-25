AC_PREREQ(2.61)
AC_INIT(pdf2djvu, 0.5.5, ubanus@users.sf.net)
AC_LANG(C++)

# Checks for programs.

AC_PROG_CXX
AC_PROG_INSTALL
AC_CHECK_TOOL(WINDRES, windres, :)

# Checks for libraries.

PKG_CHECK_MODULES(DJVULIBRE, ddjvuapi >= 3.5.17)
djvulibre_bin_path=`$PKG_CONFIG --variable exec_prefix ddjvuapi`/bin
AC_DEFINE_UNQUOTED(DJVULIBRE_BIN_PATH, "$djvulibre_bin_path", Define to the path to DjVuLibre utilities)
djvulibre_version=`$PKG_CONFIG --modversion ddjvuapi`
PACKAGE_STRING="$PACKAGE_STRING (DjVuLibre $djvulibre_version"
AC_DEFINE_UNQUOTED(DJVULIBRE_VERSION_STRING, "$djvulibre_version", Define to the version of DjVuLibre)

djvulibre_bugs=''
AC_MSG_CHECKING(DjVuLibre fitness)
for tool in cjb2 c44 csepdjvu ddjvu djvused djvuextract djvumake
do
  if test ! -x "$djvulibre_bin_path/$tool"
  then
    AC_MSG_RESULT(incomplete set of tools)
    AC_MSG_ERROR($tool not found)
  fi
done
echo 'P1 12 12' > conftest.458211.pbm
for i in 1 2 3 4 5 6 7 8 9 a b c
do
  echo '0 0 0 0 0 0 0 0 0 0 0 0' >> conftest.458211.pbm; 
done
"$djvulibre_bin_path/cjb2" conftest.458211.pbm conftest.458211.djvu
"$djvulibre_bin_path/ddjvu" -format=rle conftest.458211.djvu conftest.458211.sep
printf 'P6 1 1 255 xxx' >> conftest.458211.sep
"$djvulibre_bin_path/csepdjvu" conftest.458211.sep conftest.458211.djvu 2>/dev/null || djvulibre_bugs="$djvulibre_bugs 458211"

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

printf 'P1 3 3 0 0 0 0 0 0 0 0 0 0' > conftest.458086.pbm
"$djvulibre_bin_path/cjb2" conftest.458086.pbm conftest.458086.p1.djvu
"$djvulibre_bin_path/cjb2" conftest.458086.pbm conftest.458086.p2.djvu
printf 'AT&TFORM\0\0\0\x35DJVMDIRM\0\0\0\x29\x01\0\x02\xff\xff\xc6\xbf\x8a' > conftest.458086.djvu
printf '\x1f\xeb\x4b\xff\x40\x8e\x37\x86\x3f\x6c\xbe\x78\x02\x3c\x93\x9b' >> conftest.458086.djvu
printf '\x17\x57\x16\x67\x1d\x7a\xeb\x57\xf8\xf3\x7e\x64\x32\x42\x88\xd2\x4f' >> conftest.458086.djvu
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
poppler_version=`$PKG_CONFIG --modversion poppler-splash`
AC_DEFINE_UNQUOTED(POPPLER_VERSION_STRING, "$poppler_version", Define to the version of poppler)
PACKAGE_STRING="$PACKAGE_STRING, poppler $poppler_version"
poppler_version=`printf '%s' "$poppler_version" | sed -e 's/\(^[[0-9]]\+[[.]][[0-9]]\+$\)/\1.0/; s/[[.]]/ /g'`
poppler_version=`printf '%02d' $poppler_version | sed -e 's/^0*//g'`
AC_DEFINE_UNQUOTED(POPPLER_VERSION, $poppler_version, Define to the version of poppler, as integer)

PKG_CHECK_MODULES(GRAPHICSMAGICK, GraphicsMagick++, 
  [
    AC_DEFINE(HAVE_GRAPHICSMAGICK, 1, Define if you have GraphicsMagick++ installed)
    graphicsmagick_version=`$PKG_CONFIG --modversion GraphicsMagick++`
    AC_DEFINE_UNQUOTED(GRAPHICSMAGICK_VERSION_STRING, "$graphicsmagick_version", Define to the version of GraphicsMagick++)
    PACKAGE_STRING="$PACKAGE_STRING, GraphicsMagick++ $graphicsmagick_version)"
  ],
  [
    PACKAGE_STRING="$PACKAGE_STRING)"
  ]
)
 
AC_DEFINE_UNQUOTED(PACKAGE_STRING, "$PACKAGE_STRING", Define to the full name and version of this package)

AC_MSG_CHECKING(for PStreams)
have_pstreams=no
AC_TRY_LINK(
  [#include <pstream.h>],
  [redi::ipstream in;],
  [
    AC_DEFINE(HAVE_PSTREAM_H, 1, [Define if you have the <pstream.h> file])
    have_pstreams=yes
  ]
)
if test "$have_pstreams" = "no"
then
  AC_TRY_LINK(
    [#include <pstreams/pstream.h>],
    [redi::ipstream in;],
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

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Turn on compile warnings

if test "$GXX" = yes
then
  CXXFLAGS="$CXXFLAGS -Wall"
fi

# Output files

AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADER(version.hh)
AC_OUTPUT

# vim:ts=2 sw=2 et 
