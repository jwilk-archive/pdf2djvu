# encoding=UTF-8

# Copyright © 2009-2015 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    case,
    re,
)

class test(case):

    def test_11(self):
        # http://sourceforge.net/p/djvu/bugs/106/
        # fixed in [5e3937296514]
        self.pdf2djvu('--bg-subsample=11', '--dpi=72').assert_()
        r = self.djvudump()
        r.assert_(stdout=re('BG44.* 10x11$', re.M))

    def test_12(self):
        # https://bugs.debian/458211
        # Prior to pdf2djvu 0.5.0, subsample ratio 12 was not allowed.
        # Now we require a fixed verison of DjVuLibre.
        self.pdf2djvu('--bg-subsample=12', '--dpi=72').assert_()
        r = self.djvudump()
        r.assert_(stdout=re('BG44.* 9x9$', re.M))

# vim:ts=4 sts=4 sw=4 et
