# encoding=UTF-8

# Copyright © 2015-2017 Jakub Wilk <jwilk@jwilk.net>
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

import re

from tools import (
    case,
)

class test(case):

    # Bug: https://github.com/jwilk/pdf2djvu/issues/112
    # + fixed in 0.9.2 [74a8915ca7602adfd0e71c7d329de2265eec4532]

    def test(self):
        r = self.pdf2djvu()
        r.assert_(
            stderr=re.compile(''),
            stdout=re.compile(''),  # https://bugs.freedesktop.org/show_bug.cgi?id=81513
        )

# vim:ts=4 sts=4 sw=4 et
