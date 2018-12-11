# encoding=UTF-8

# Copyright © 2009-2018 Jakub Wilk <jwilk@jwilk.net>
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
    # Bug: https://github.com/jwilk/pdf2djvu/issues/20
    # + fixed in 0.5.4 [46b70e11778f984c2db302a1c8a18bc0996a387c]

    def test_no_crop(self):
        self.pdf2djvu().assert_()
        r = self.print_text()
        r.assert_(stdout=re.compile('^Lorem ipsum *\n'))

    def test_crop(self):
        self.pdf2djvu('--crop-text').assert_()
        r = self.print_text()
        r.assert_(stdout=re.compile('^Lorem *\n'))

# vim:ts=4 sts=4 sw=4 et
