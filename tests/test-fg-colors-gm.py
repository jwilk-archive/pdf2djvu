# encoding=UTF-8

# Copyright © 2010-2022 Jakub Wilk <jwilk@jwilk.net>
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
    assert_equal,
    assert_in,
    case,
    count_colors,
)

class test(case):

    def test(self):
        # Bug: https://github.com/jwilk/pdf2djvu/issues/47
        # + fixed in 0.7.2 [a10cfc8ac94e8f3b7e01e089056bf6a371d1a68d]
        def t(i, n):
            self.require_feature('GraphicsMagick')
            self.pdf2djvu(
                '--dpi=72',
                '--fg-colors={0}'.format(i)
            ).assert_()
            image = self.decode()
            image = self.decode(mode='foreground')
            colors = count_colors(image)
            if isinstance(n, tuple):
                assert_in(len(colors), n)
            else:
                assert_equal(len(colors), n)
        yield t, 1, 2
        yield t, 2, 3
        yield t, 4, 5
        yield t, 255, 241
        yield t, 256, (245, 256)
        yield t, 652, (245, 325)

    def test_range_error(self):
        def t(i):
            self.require_feature('GraphicsMagick')
            r = self.pdf2djvu('--fg-colors={0}'.format(i))
            msg = 'The specified number of foreground colors is outside the allowed range: 1 .. 4080'
            r.assert_(
                stderr=re.compile('^' + re.escape(msg) + '\n'),
                rc=1,
            )
        t('-1')
        t(0)
        t(4081)

    def test_bad_number(self):
        def t(i):
            self.require_feature('GraphicsMagick')
            r = self.pdf2djvu('--fg-colors={0}'.format(i))
            r.assert_(
                stderr=re.compile('^"{0}" is not a valid number\n'.format(i)),
                rc=1,
            )
        t('')
        t('1x')
        t('0x1')
        t(23 ** 17)

# vim:ts=4 sts=4 sw=4 et
