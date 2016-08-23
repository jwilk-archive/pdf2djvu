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

    def t(self, page, url, border='(xor)'):
        result = self.print_ant(page=page)
        template = '(maparea "{url}" "" (rect NNN NNN NNN NNN) {border})'.format(
            url=url,
            border=border,
        )
        regex = re.escape(template).replace('NNN', '[0-9]+')
        result.assert_(stdout=re(regex))

    def test(self):
        # Bug: https://bitbucket.org/jwilk/pdf2djvu/issues/3
        # + fixed in 0.4.10 [a244b65e0661]
        # + fixed in 0.4.12 [2e5ab20f8a07]
        self.pdf2djvu().assert_()
        self.t(1, '#p0002.djvu')
        self.t(2, '#p0001.djvu', '(border #ff7f00)')
        self.t(3, 'http://www.example.org/')

    def test_border_avis(self):
        def t(*args):
            self.pdf2djvu(*args).assert_()
            self.t(1, '#p0002.djvu', '(xor) (border_avis)')
            self.t(2, '#p0001.djvu', '(border #ff7f00) (border_avis)')
            self.t(3, 'http://www.example.org/', '(xor) (border_avis)')
        t('--hyperlinks', 'border-avis')
        t('--hyperlinks', 'border_avis')

    def test_border_color(self):
        self.pdf2djvu('--hyperlinks', '#3742ff').assert_()
        self.t(1, '#p0002.djvu', '(border #3742ff)')
        self.t(2, '#p0001.djvu', '(border #3742ff)')
        self.t(3, 'http://www.example.org/', '(border #3742ff)')

    def test_none(self):
        def t(*args):
            self.pdf2djvu(*args).assert_()
            for n in range(3):
                r = self.print_ant(page=(n + 1))
                r.assert_(stdout='')
        t('--hyperlinks', 'none')
        t('--no-hyperlinks')

    def test_bad_argument(self):
        r = self.pdf2djvu('--hyperlinks', 'off')
        r.assert_(stderr=re('^Unable to parse hyperlinks options\n'), rc=1)

# vim:ts=4 sts=4 sw=4 et
