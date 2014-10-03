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
    fixed in [90111a528eb5]
    '''
    def test(self):
        title = r'\316\232\317\211\316\264\316\271\316\272\316\277\317\200\316\277\316\257\316\267\317\203\316\267 \317\207\316\261\317\201\316\261\316\272\317\204\316\256\317\201\317\211\316\275'
        self.pdf2djvu('--dpi=72').assert_()
        r = self.print_meta()
        r.assert_(stdout=re('Title\t"%s"' % title.replace('\\', '\\\\')))

# vim:ts=4 sts=4 sw=4 et
