# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import unicodedata

from common import (
    case,
    re,
)

class test(case):
    '''
    fixed in [18b2ae04de2f], [fa7d4addf18e]
    '''

    text = u'\N{LATIN SMALL LIGATURE FL}uorogra\N{LATIN SMALL LIGATURE FI}a'
    text_nfkc = unicodedata.normalize('NFKC', text).encode('UTF-8')
    text_no_nfkc = text.encode('UTF-8')

    def test_nfkc(self):
        self.pdf2djvu().assert_()
        r = self.print_pure_txt()
        r.assert_(stdout=re('^%s *$' % self.text_nfkc, re.M))

    def test_no_nfkc(self):
        self.pdf2djvu('--no-nfkc').assert_()
        r = self.print_pure_txt()
        r.assert_(stdout=re('^%s *$' % self.text_no_nfkc, re.M))

# vim:ts=4 sw=4 et
