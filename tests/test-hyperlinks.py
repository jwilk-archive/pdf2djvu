# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

import re

from common import (
    case,
)

class test(case):
    '''
    fixed in [a244b65e0661], [2e5ab20f8a07]
    '''
    def test(self):
        self.pdf2djvu().assert_()
        r = self.print_ant(page=1)
        r.assert_(stdout=re.compile(
            r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)][)]$',
        ))
        r = self.print_ant(page=2)
        r.assert_(stdout=re.compile(
            r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #ff7f00[)][)]$',
        ))

# vim:ts=4 sw=4 et
