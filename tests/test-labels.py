# encoding=UTF-8

# Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
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

from tools import (
    case,
    re,
)

class test(case):
    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issue/109
    # + fixed in 0.9
    def test(self):
        self.pdf2djvu('--page-title-template', '{label}').assert_()
        r = self.ls()
        r.assert_(stdout=re(
            '\n'
            '\s*1\s+P\s+\d+\s+[\w.]+\s+T=one\n'
            '\s*2\s+P\s+\d+\s+[\w.]+\s+T=Αʹ\n'
            '\s*3\s+P\s+\d+\s+[\w.]+\s+T=i\n'
            '\s*4\s+P\s+\d+\s+[\w.]+\s+T=1\n'
        ))

# vim:ts=4 sts=4 sw=4 et
