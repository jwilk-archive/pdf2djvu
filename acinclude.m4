dnl | Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
dnl |
dnl | This package is free software; you can redistribute it and/or modify
dnl | it under the terms of the GNU General Public License as published by
dnl | the Free Software Foundation; version 2 dated June, 1991.

AC_DEFUN(
    [P_MAYBE_ADD_CXXFLAG],
    [
        copy_CXXFLAGS="$CXXFLAGS"
        CXXFLAGS="$CXXFLAGS $1"
        AC_MSG_CHECKING([whether $CXX accepts $1])
        AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM()],
            [AC_MSG_RESULT([yes])],
            [
                AC_MSG_RESULT([no])
                CXXFLAGS=$copy_CXXFLAGS
            ]
        )
    ]
)

dnl vim:ts=4 sts=4 sw=4 et ft=config
