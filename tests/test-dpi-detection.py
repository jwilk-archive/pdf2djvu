# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import re

from common import (
    case,
)

class test(case):
    '''
    http://sf.net/tracker/?func=detail&aid=1972089&group_id=32953&atid=406583
    fixed in [5e3937296514]
    '''
    def test(self):
        self.pdf2djvu('--guess-dpi').assert_()
        r = self.djvudump()
        r.assert_(stdout=re.compile('INFO .* DjVu 100x200,'))

# vim:ts=4 sts=4 sw=4 et
