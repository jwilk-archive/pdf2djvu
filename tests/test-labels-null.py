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

    def test(self):
        self.pdf2djvu().assert_()
        r = self.ls()
        r.assert_(stdout=re.compile(
            ur'\n'
            ur'\s*1\s+P\s+\d+\s+[\w.]+\s+T=\uFFFDnul\uFFFDl\uFFFD\n'
            ur'\s*2\s+P\s+\d+\s+[\w.]+\s+T=1\n'.encode('UTF-8')
        ))

# vim:ts=4 sts=4 sw=4 et
