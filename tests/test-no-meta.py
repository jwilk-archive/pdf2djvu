# encoding=UTF-8

# Copyright © 2009-2012 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdfdjvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

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

# vim:ts=4 sts=4 sw=4 et
