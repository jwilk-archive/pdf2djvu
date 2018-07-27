# encoding=UTF-8

# Copyright © 2018 Jakub Wilk <jwilk@jwilk.net>
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
)

class test(case):
    def test(self):
        # Bug: https://github.com/jwilk/pdf2djvu/issues/131
        # + fixed in 0.9.10 [1359ec9def9173d33a5eaca19ab08ce11d2a1306]
        self.pdf2djvu('--no-render', '--dpi=72', '-j', '8').assert_()

# vim:ts=4 sts=4 sw=4 et
