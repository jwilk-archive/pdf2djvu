dnl | Copyright © 2015-2018 Jakub Wilk <jwilk@jwilk.net>
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
        p_CXXFLAGS="$CXXFLAGS"
        if test -z "$p_cflags_Werror_unknown_warning_option"
        then
            CXXFLAGS="$p_CXXFLAGS -Werror=unknown-warning-option"
            AC_CACHE_CHECK([whether $CXX accepts -Werror=unknown-warning-option], [pdf2djvu_cv_cflags_Werror_unknown_warning_option], [
                AC_COMPILE_IFELSE(
                    [AC_LANG_PROGRAM()],
                    [pdf2djvu_cv_cflags_Werror_unknown_warning_option=yes],
                    [pdf2djvu_cv_cflags_Werror_unknown_warning_option=no],
                )
            ])
        fi
        CXXFLAGS="$p_CXXFLAGS"
        p_cflags_Werror_unknown_warning_option="$pdf2djvu_cv_cflags_Werror_unknown_warning_option"
        if test "$p_cflags_Werror_unknown_warning_option" = yes
        then
            CXXFLAGS="$CXXFLAGS -Werror=unknown-warning-option"
        fi
        CXXFLAGS="$CXXFLAGS $1"
        AC_CACHE_CHECK([whether $CXX accepts $1], [AS_TR_SH([pdf2djvu_cv_cflags$1])], [
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM()],
                [AS_TR_SH([pdf2djvu_cv_cflags$1])=yes],
                [AS_TR_SH([pdf2djvu_cv_cflags$1])=no]
            )
        ])
        if test "$AS_TR_SH([pdf2djvu_cv_cflags$1])" = yes
        then
            CXXFLAGS="$p_CXXFLAGS $1"
        else
            CXXFLAGS="$p_CXXFLAGS"
        fi
    ]
)

AC_DEFUN(
    [P_MAYBE_ADD_CXXFLAGS],
    [
        m4_foreach(
            [p_flag], [$@],
            [m4_ifval(
                p_flag,
                [P_MAYBE_ADD_CXXFLAG(p_flag)],
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
        AC_CACHE_CHECK([for $3], [pdf2djvu_cv_have_$3], [
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([[$1]], [[$2 (*f)($4) = $3;]])],
                [pdf2djvu_cv_have_$3=yes],
                [pdf2djvu_cv_have_$3=no],
            )
        ])
        if test $pdf2djvu_cv_have_$3 = yes
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

# AX_CXX_COMPILE_STDCXX_11 is too thorough. It would reject GCC 4.6, even
# though this compiler implements enough C++11 bits for our purposes.

m4_define([_P_CXX11_CODE], [
    void* test()
    {
        auto t = nullptr;
        return t;
    }
])

AC_DEFUN([_P_CXX11], [
    if test $p_have_cxx11 = no
    then
        p_cxx="$CXX"
        p_CXXFLAGS="$CXXFLAGS"
        m4_ifval([$1], [
            p_cxx_opt="-std=$1"
            p_cxx="$CXX $p_cxx_opt"
            CXXFLAGS="$CXXFLAGS $p_cxx_opt"
        ])
        AC_CACHE_CHECK([whether $p_cxx supports C++11], [AS_TR_SH([pdf2djvu_cv_cxx11_$1])], [
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([_P_CXX11_CODE])],
                [AS_TR_SH([pdf2djvu_cv_cxx11_$1])=yes],
                [AS_TR_SH([pdf2djvu_cv_cxx11_$1])=no]
            )
        ])
        CXXFLAGS="$p_CXXFLAGS"
        if test $AS_TR_SH([pdf2djvu_cv_cxx11_$1]) = yes
        then
            p_have_cxx11=yes
            CPPFLAGS="$CPPFLAGS $p_cxx_opt"
        fi
    fi
])

# P_CXX11()

AC_DEFUN([P_CXX11], [
    p_have_cxx11=no
    _P_CXX11()
    _P_CXX11([gnu++11])
    _P_CXX11([gnu++0x])
    if test $p_have_cxx11 = no
    then
        AC_MSG_ERROR([the compiler does not support C++11])
    fi
])

dnl vim:ts=4 sts=4 sw=4 et ft=config
