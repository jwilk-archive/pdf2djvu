# encoding=UTF-8

# Copyright © 2011, 2012 Jakub Wilk
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
    https://code.google.com/p/pdf2djvu/issues/detail?id=59
    fixed in [6c155e7cc346]
    '''
    def test(self):
        self.pdf2djvu('--monochrome').assert_()
        r = self.djvudump()
        r.assert_(stdout=re.compile(r'Sjbz \[[0-9]{4,}\]'))

# vim:ts=4 sw=4 et
