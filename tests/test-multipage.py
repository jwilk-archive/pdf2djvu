# encoding=UTF-8

# Copyright © 2010-2017 Jakub Wilk <jwilk@jwilk.net>
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
    # Bug: https://github.com/jwilk/pdf2djvu/issues/50
    # + fixed in 0.7.4 [1c373918e0152d452c24936818591d32df0ff7fc]

    def test(self):
        self.pdf2djvu('--pages=2').assert_()
        r = self.print_text()
        r.assert_(stdout=re.compile('^ipsum *\n'))

# vim:ts=4 sts=4 sw=4 et
