# encoding=UTF-8

# Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
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

from tools import (
    case,
    re,
)

class test(case):

    def test(self):
        # Bug: https://bitbucket.org/jwilk/pdf2djvu/issue/3
        # + fixed in 0.4.10 [a244b65e0661]
        # + fixed in 0.4.12 [2e5ab20f8a07]
        self.pdf2djvu().assert_()
        r = self.print_ant(page=1)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)][)]$',
        ))
        r = self.print_ant(page=2)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #ff7f00[)][)]$',
        ))

    def test_border_avis(self):
        self.pdf2djvu('--hyperlinks=border-avis').assert_()
        r = self.print_ant(page=1)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)] [(]border_avis[)][)]$',
        ))
        r = self.print_ant(page=2)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #ff7f00[)] [(]border_avis[)][)]$',
        ))

    def test_border_color(self):
        self.pdf2djvu('--hyperlinks=#3742ff').assert_()
        r = self.print_ant(page=1)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #3742ff[)][)]$',
        ))
        r = self.print_ant(page=2)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #3742ff[)][)]$',
        ))

# vim:ts=4 sts=4 sw=4 et
