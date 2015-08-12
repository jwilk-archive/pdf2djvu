# encoding=UTF-8

# Copyright © 2014 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdfdjvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

from common import (
    SkipTest,
    case,
    re,
)

class test(case):
    '''
    https://bugs.debian.org/760396
    '''
    def test(self):
        self.pdf2djvu('--dpi=72').assert_()
        r = self.djvudump()
        try:
            r.assert_(stdout=re(r'\A(\s+(?!FG)\S+.*\n)+\Z'))
        except AssertionError:
            raise SkipTest('https://bugs.freedesktop.org/show_bug.cgi?id=68360')

# vim:ts=4 sts=4 sw=4 et
