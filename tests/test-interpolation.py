# encoding=UTF-8

# Copyright © 2014 Jakub Wilk
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
    https://bugs.debian.org/760396
    '''
    def test(self):
        self.pdf2djvu().assert_()
        r = self.djvudump()
        r.assert_(stdout=re(r'\A(\s+(?!FG)\S+.*\n)+\Z'))

# vim:ts=4 sts=4 sw=4 et
