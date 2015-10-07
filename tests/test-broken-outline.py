# encoding=UTF-8

# Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdf2djvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

from tools import (
    case,
    re,
)

class test(case):

    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issues/112
    # + fixed in 0.9.2 [a64d38473b5e]

    def test(self):
        r = self.pdf2djvu()
        r.assert_(
            stderr=re(''),
            stdout=re(''),  # https://bugs.freedesktop.org/show_bug.cgi?id=81513
        )

# vim:ts=4 sts=4 sw=4 et
