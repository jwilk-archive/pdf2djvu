# encoding=UTF-8

# Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
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
    assert_equal,
    case,
    re,
)

class test(case):
    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issues/98
    # + fixed in 0.8 [e98f3c4cfa2e]

    def test_overwrite(self):
        pdf_path = self.get_pdf_path()
        with open(pdf_path, 'rb') as pdf_file:
            pdf_before = pdf_file.read()
        cmdline = (self.get_pdf2djvu_command() + (
            '-q',
            self.get_pdf_path(),
            '-o', self.get_pdf_path()
        ))
        r = self.run(*cmdline)
        r.assert_(stderr=re('Input file is the same as output file:'), rc=1)
        with open(pdf_path, 'rb') as pdf_file:
            pdf_after = pdf_file.read()
        assert_equal(pdf_before, pdf_after)

# vim:ts=4 sts=4 sw=4 et
