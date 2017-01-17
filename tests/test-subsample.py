# encoding=UTF-8

# Copyright © 2009-2017 Jakub Wilk <jwilk@jwilk.net>
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

    def test_11(self):
        # Bug: https://sourceforge.net/p/djvu/bugs/106/
        # + fixed in 0.4.11 [25f63fdcee01a93df16fcd56ebd7587165f4ee52]
        self.pdf2djvu('--bg-subsample=11', '--dpi=72').assert_()
        r = self.djvudump()
        r.assert_(stdout=re.compile('BG44.* 10x11$', re.M))

    def test_12(self):
        # Bug: https://bugs.debian.org/458211
        #
        # Prior to pdf2djvu 0.5.0, subsample ratio 12 was not allowed.
        # Now we require a fixed version of DjVuLibre.
        self.pdf2djvu('--bg-subsample=12', '--dpi=72').assert_()
        r = self.djvudump()
        r.assert_(stdout=re.compile('BG44.* 9x9$', re.M))

# vim:ts=4 sts=4 sw=4 et
