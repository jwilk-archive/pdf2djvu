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

from tools import (
    assert_equal,
    case,
    count_colors,
)

class test(case):
    # Bug: https://github.com/jwilk/pdf2djvu/issues/45
    # + fixed in 0.7.2 [72be13a3bc33817030125301fa34df3ddef63a10]

    def test(self):
        yield self._test, 'default', 325
        yield self._test, 'web', 43
        yield self._test, 'black', 2

    def _test(self, method, n):
        self.pdf2djvu(
            '--dpi=72',
            '--fg-colors={method}'.format(method=method)
        ).assert_()
        image = self.decode()
        image = self.decode(mode='foreground')
        colors = count_colors(image)
        assert_equal(len(colors), n)

# vim:ts=4 sts=4 sw=4 et
