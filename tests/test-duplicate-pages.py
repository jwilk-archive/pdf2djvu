# encoding=UTF-8

# Copyright © 2009-2014 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import re

from common import (
    assert_not_equal,
    case,
)

class test(case):
    '''
    https://code.google.com/p/pdf2djvu/issues/detail?id=23
    fixed in [ede3c622526a]
    '''
    def test_bundled(self):
        r = self.pdf2djvu('--pages=1,1')
        r.assert_(stderr=re.compile('^Duplicate page identifier:', re.M), rc=None)
        assert_not_equal(r.rc, 0)

    def test_indirect(self):
        r = self.pdf2djvu_indirect('--pages=1,1')
        r.assert_(stderr=re.compile('^Duplicate page identifier:', re.M), rc=None)
        assert_not_equal(r.rc, 0)

# vim:ts=4 sts=4 sw=4 et
