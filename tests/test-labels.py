# encoding=UTF-8

# Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
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

    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issue/109
    # + fixed in 0.9 [dc1734ad4946]

    def test(self):
        def t(*args):
            self.pdf2djvu(*args).assert_()
            r = self.ls()
            r.assert_(stdout=re(
                '\n'
                '\s*1\s+P\s+\d+\s+[\w.]+\s+T=one\n'
                '\s*2\s+P\s+\d+\s+[\w.]+\s+T=Αʹ\n'
                '\s*3\s+P\s+\d+\s+[\w.]+\s+T=i\n'
                '\s*4\s+P\s+\d+\s+[\w.]+\s+T=1\n'
            ))
        t()
        t('--page-title-template', '{label}')

    def test_arithmetic(self):
        def t(offset):
            r = self.pdf2djvu('--page-title-template', '{label' + offset + '}')
            r.assert_(
                stderr='Unable to format field {label}: type error: expected number, not string\n',
                rc=1
            )
        t('+1')
        t('-1')

    def test_auto_width(self):
        r = self.pdf2djvu('--page-title-template', '{label:0*}')
        r.assert_(
            stderr='Unable to format field {label}: unknown maximum width\n',
            rc=1
        )

    def test_page_id(self):
        # {label} can be used in --page-title-template,
        # but not it --page-id-template
        r = self.pdf2djvu('--page-id-template', '{label}')
        r.assert_(
            stderr='Unable to format field {label}: no such variable\n',
            rc=1
        )

# vim:ts=4 sts=4 sw=4 et
