# encoding=UTF-8

# Copyright © 2009-2012 Jakub Wilk <jwilk@jwilk.net>
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
    case,
    re,
)

class test(case):
    '''
    fixed in [9a9650e7f06f]
    '''
    def test(self):
        self.pdf2djvu().assert_()
        r = self.djvudump()
        r.assert_(stdout=re(r'BG44 \[[0-9][0-9]\] .* 75x75'))

# vim:ts=4 sts=4 sw=4 et
