# encoding=UTF-8

# Copyright © 2015-2018 Jakub Wilk <jwilk@jwilk.net>
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

    # Bug: https://github.com/jwilk/pdf2djvu/issues/113
    # + fixed in 0.9.5 [1b262b90854cd3d5359cd7fdf6b72642b54c8f60]
    def test(self):
        r = self.pdf2djvu('--page-title-template', '{label}', quiet=False)
        r.assert_(stderr=re.compile('Warning: Ignoring duplicate page title: x\n'))
        r = self.ls()
        r.assert_(stdout=re.compile(
            r'\n'
            r'\s*1\s+P\s+\d+\s+[\w.]+\s+T=x\n'
            r'\s*2\s+P\s+\d+\s+[\w.]+\n'
            r'\s*3\s+P\s+\d+\s+[\w.]+\n'
        ))

# vim:ts=4 sts=4 sw=4 et
