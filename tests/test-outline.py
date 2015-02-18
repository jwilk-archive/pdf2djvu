# encoding=UTF-8

# Copyright © 2015 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
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

# vim:ts=4 sts=4 sw=4 et

