# encoding=UTF-8

# Copyright © 2022 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdf2djvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

from __future__ import print_function

import itertools

from tools import (
    assert_fail,
    case,
)

def print_pgm(image):
    width = len(image[0])
    print('+', '-' * (2 * width), '+', sep='')
    for line in image:
        print('|', end='')
        for px in line:
            if px == '\xff':
                print('  ', end='')
            elif px == '\0':
                print('[]', end='')
            else:
                print('()', end='')
        print('|')
    print('+', '-' * (2 * width), '+', sep='')

class test(case):
    # Bug: https://github.com/jwilk/pdf2djvu/issues/154

    def test(self):
        self.pdf2djvu('--dpi=150')
        image = self.decode(fmt='pgm')
        print_pgm(image)
        for line in image:
            sig = str.join('', (pixel for (pixel, _) in itertools.groupby(line)))
            if sig not in {'\xff', '\xff\x00\xff', '\xff\x00\xff\x00\xff'}:
                assert_fail('image does not look like uppercase delta')

# vim:ts=4 sts=4 sw=4 et
