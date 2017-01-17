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
    def test_rot13(self):
        self.require_feature('POSIX')
        self.pdf2djvu('--filter-text', 'tr "a-zA-Z" "n-za-mN-ZA-M"').assert_()
        r = self.print_text()
        r.assert_(stdout=re.compile('^Yberz vcfhz *\n'))

# vim:ts=4 sts=4 sw=4 et
