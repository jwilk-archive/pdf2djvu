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
    assert_greater,
    count_ppm_colors,
)

class test(case):
    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issues/115
    # + introduced in 0.7.1 [3eea6fbdf75b]
    # + introduced in 0.7.10 [6c155e7cc346]
    # + fixed in 0.9.3 [369a24e7f845]

    def test(self):
        self.pdf2djvu()
        r = self.decode()
        r.assert_(stdout=None)
        r = self.decode(mode='foreground')
        r.assert_(stdout=None)
        colors = count_ppm_colors(r.stdout)
        assert_greater(colors.get('\xff\0\0', 0), 5000)

# vim:ts=4 sts=4 sw=4 et
