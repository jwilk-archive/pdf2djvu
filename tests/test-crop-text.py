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
    http://code.google.com/p/pdf2djvu/issues/detail?id=20
    fixed in [c06a41afdc46]
    '''
    def test_no_crop(self):
        self.pdf2djvu().assert_()
        r = self.print_pure_txt()
        r.assert_(stdout=re.compile('^Lorem ipsum *\n'))

    def test_crop(self):
        self.pdf2djvu('--crop-text').assert_()
        r = self.print_pure_txt()
        r.assert_(stdout=re.compile('^Lorem *\n'))

# vim:ts=4 sw=4 et
