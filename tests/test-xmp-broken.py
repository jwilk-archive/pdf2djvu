# encoding=UTF-8

# Copyright © 2009-2021 Jakub Wilk <jwilk@jwilk.net>
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
    assert_regex,
    case,
)

class test(case):
    def test_verbatim(self):
        self.pdf2djvu('--verbatim-metadata').assert_()
        xmp = self.extract_xmp()
        assert_regex(xmp, '<broken')

    def test_no_verbatim(self):
        self.require_feature('Exiv2')
        r = self.pdf2djvu()
        r.assert_(stderr=re.compile(r'\AXMP metadata error: XMP Toolkit error 201: .*\n\Z'))
        xmp = self.extract_xmp()
        assert_regex(xmp, '<broken')

# vim:ts=4 sts=4 sw=4 et
