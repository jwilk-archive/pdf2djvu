# encoding=UTF-8

# Copyright © 2010-2015 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    case,
)

class test(case):
    '''
    https://bitbucket.org/jwilk/pdf2djvu/issue/47
    fixed in [3d0f55ae5a65]
    '''
    '''
    https://bitbucket.org/jwilk/pdf2djvu/issue/45
    fixed in [fc7df6c4d3d3]
    '''
    def test(self):
        for i in [1, 2, 4, 255, 256, 652]:
            yield self._test, i

    def _test(self, i):
        self.require_feature('GraphicsMagick')
        self.pdf2djvu('--fg-colors=%d' % i).assert_()
        r = self.decode()
        r.assert_(stdout=None)

# vim:ts=4 sts=4 sw=4 et
