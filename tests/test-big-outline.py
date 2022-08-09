# encoding=UTF-8

# Copyright © 2016-2022 Jakub Wilk <jwilk@jwilk.net>
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

b = 1000
item_template = '''
  ("ipsum {0}"
   "#p0001.djvu" )
'''.strip('\n')
expected_outline_sexpr = ('''\
(bookmarks
 ("Lorem"
  "#p0001.djvu"
'''
+ str.join('\n', (item_template.format(i) for i in range(0, b)))
+ ' ) )\n'
)

class test(case):

    def test_multi_page(self):
        self.pdf2djvu().assert_()
        self.print_outline().assert_(stdout=expected_outline_sexpr)

# vim:ts=4 sts=4 sw=4 et
