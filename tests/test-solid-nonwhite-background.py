# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

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
        r.assert_(stdout=re('BG44 \[[0-9][0-9]\] .* 75x75'))

# vim:ts=4 sts=4 sw=4 et
