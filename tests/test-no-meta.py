# encoding=UTF-8

# Copyright © 2009, 2011, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    case,
)

class test(case):
    '''
    fixed in [d1b09854a9bc]
    '''
    def test(self):
        self.pdf2djvu('--no-metadata', '--dpi=72').assert_()
        self.print_meta().assert_()

# vim:ts=4 sw=4 et
