dnl | Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
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

AC_DEFUN(
    [P_MAYBE_ADD_CXXFLAGS],
    [
        m4_foreach(
            [p_flag], [$@],
            [m4_ifval(
                p_flag,
                [P_MAYBE_ADD_CXXFLAG([p_flag])],
                []
            )]
        )
    ]
)

# P_CHECK_FUNC(
#   PROLOGUE, RETURN_TYPE, FUNCTION, ARGUMENTS,
#   [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND]
# )

AC_DEFUN(
    [P_CHECK_FUNC],
    [
        AC_CACHE_CHECK([for $3], [_cv_p_have_$3], [
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([[$1]], [[$2 (*f)($4) = $3;]])],
                [_cv_p_have_$3=yes],
                [_cv_p_have_$3=no],
            )
        ])
        if test $_cv_p_have_$3 = yes
        then
            m4_default([$5], [
                AC_DEFINE(
                    AS_TR_CPP([HAVE_$3]), 1,
                    [Define to 1 if you have the '$2 $3($4)' function.]
                )
            ])
        else
            m4_default([$6], [:])
        fi
    ]
)

dnl vim:ts=4 sts=4 sw=4 et ft=config
