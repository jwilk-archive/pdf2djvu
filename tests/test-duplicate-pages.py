# encoding=UTF-8

# Copyright © 2009-2018 Jakub Wilk <jwilk@jwilk.net>
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
    assert_not_equal,
    case,
)

class test(case):
    # Bug: https://github.com/jwilk/pdf2djvu/issues/23
    # + fixed in 0.5.5 [ad11da594a254f203f603fd4105fb05c8b508c64]

    # Bug: https://github.com/jwilk/pdf2djvu/issues/73
    # + fixed in 0.8 [5d5fa0e235497aa7e82ea2ba5fcfd3351ae53562]

    def test_bundled(self):
        r = self.pdf2djvu('--pages=1,1')
        r.assert_(stderr=re.compile('^Duplicate page:', re.M), rc=None)
        assert_not_equal(r.rc, 0)

    def test_indirect(self):
        r = self.pdf2djvu_indirect('--pages=1,1')
        r.assert_(stderr=re.compile('^Duplicate page:', re.M), rc=None)
        assert_not_equal(r.rc, 0)

# vim:ts=4 sts=4 sw=4 et
