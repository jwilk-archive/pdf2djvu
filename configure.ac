AC_PREREQ(2.61)
AC_INIT(pdf2djvu, 0.5.0, ubanus@users.sf.net)
AC_LANG(C++)

# Checks for programs.

AC_PROG_CXX

# Checks for libraries.

AC_SEARCH_LIBS(iconv, iconv)

PKG_CHECK_MODULES(DJVULIBRE, ddjvuapi >= 3.5.17)
djvulibre_bin_path=`$PKG_CONFIG --variable exec_prefix ddjvuapi`
AC_DEFINE_UNQUOTED(DJVULIBRE_BIN_PATH, "$djvulibre_bin_path/bin", Define to the path to DjVuLibre utilities)
djvulibre_version=`$PKG_CONFIG --modversion ddjvuapi`
PACKAGE_STRING="$PACKAGE_STRING (DjVuLibre $djvulibre_version"
AC_DEFINE_UNQUOTED(DJVULIBRE_VERSION_STRING, "$djvulibre_version", Define to the version of DjVuLibre)

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
