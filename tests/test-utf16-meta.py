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
    # + fixed in 0.3.3 [0d4169e1bdffb34ebcef51f29abc744ade9c1556]

    def test(self):
        title = (
            r'\316\232\317\211\316\264\316\271\316\272\316\277\317\200\316\277\316\257\316\267\317\203\316\267 '
            r'\317\207\316\261\317\201\316\261\316\272\317\204\316\256\317\201\317\211\316\275'
        )
        self.pdf2djvu('--dpi=72').assert_()
        r = self.print_meta()
        r.assert_(stdout=re.compile('Title\t"{s}"'.format(s=title.replace('\\', '\\\\'))))

# vim:ts=4 sts=4 sw=4 et
