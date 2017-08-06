# encoding=UTF-8

# Copyright © 2015-2016 Jakub Wilk <jwilk@jwilk.net>
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

from tools import (
    case,
)

expected_outline_sexpr = '''\
(bookmarks
 ("Lorem"
  "#p0001.djvu"
  ("ipsum"
   "#p0002.djvu"
   ("dolor"
    "#p0001.djvu" )
   ("sit"
    "#p0002.djvu" ) )
  ("amet"
   "#p0001.djvu" )
  ("consectetur adipisci"
   "#p0002.djvu" ) )
 ("velit"
  "#p0001.djvu" ) )
'''

class test(case):

    def test_multi_page(self):
        self.pdf2djvu().assert_()
        self.print_outline().assert_(stdout=expected_outline_sexpr)

    def test_single_page(self):
        # Make sure that outline is preserved in single-page documents without
        # shared annotation chunk.
        self.pdf2djvu('-p1', '--no-metadata').assert_()
        self.print_outline().assert_(stdout=expected_outline_sexpr)

    def test_iff_corruption(self):
        # Make sure that the NAVM chunk begins on an even byte.
        #
        # Bug: https://github.com/jwilk/pdf2djvu/issues/110
        # + introduced in 0.7.20
        # + fixed in 0.8.2 [cef7b917bf1cde70884d444a4187fcebeec10ba8]
        #
        # This particular choice of options seems to trigger odd-sized DIRM chunk:
        self.pdf2djvu('-p1', '--no-metadata', '--page-title-template', 'xx').assert_()
        self.print_outline().assert_(stdout=expected_outline_sexpr)

# vim:ts=4 sts=4 sw=4 et
