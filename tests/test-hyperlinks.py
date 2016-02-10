# encoding=UTF-8

# Copyright © 2009-2016 Jakub Wilk <jwilk@jwilk.net>
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
        r = self.print_ant(page=3)
        r.assert_(stdout=re(
            r'^[(]maparea "http://www[.]example[.]org/" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)][)]$',
        ))

    def test_border_avis(self):
        def t(*args):
            self.pdf2djvu(*args).assert_()
            r = self.print_ant(page=1)
            r.assert_(stdout=re(
                r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)] [(]border_avis[)][)]$',
            ))
            r = self.print_ant(page=2)
            r.assert_(stdout=re(
                r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #ff7f00[)] [(]border_avis[)][)]$',
            ))
            r = self.print_ant(page=3)
            r.assert_(stdout=re(
                r'^[(]maparea "http://www[.]example[.]org/" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]xor[)] [(]border_avis[)][)]$',
            ))
        t('--hyperlinks', 'border-avis')
        t('--hyperlinks', 'border_avis')

    def test_border_color(self):
        self.pdf2djvu('--hyperlinks', '#3742ff').assert_()
        r = self.print_ant(page=1)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0002[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #3742ff[)][)]$',
        ))
        r = self.print_ant(page=2)
        r.assert_(stdout=re(
            r'^[(]maparea "#p0001[.]djvu" "" [(]rect [0-9]+ [0-9]+ [0-9]+ [0-9]+[)] [(]border #3742ff[)][)]$',
        ))

    def test_none(self):
        def t(*args):
            self.pdf2djvu(*args).assert_()
            for n in (1, 2):
                r = self.print_ant(page=n)
                r.assert_(stdout='')
        t('--hyperlinks', 'none')
        t('--no-hyperlinks')

    def test_bad_argument(self):
        r = self.pdf2djvu('--hyperlinks', 'off')
        r.assert_(stderr=re('^Unable to parse hyperlinks options\n'), rc=1)

# vim:ts=4 sts=4 sw=4 et
