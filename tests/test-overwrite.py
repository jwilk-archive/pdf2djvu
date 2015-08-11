# encoding=UTF-8

# Copyright © 2015 Jakub Wilk <jwilk@jwilk.net>
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    assert_equal,
    case,
    re,
)

class test(case):
    '''
    fixed in [e98f3c4cfa2e]
    '''
    def test_overwrite(self):
        pdf_path = self.get_pdf_path()
        with open(pdf_path, 'rb') as pdf_file:
            pdf_before = pdf_file.read()
        self.run(*(self.get_pdf2djvu_command() + (
            '-q',
            self.get_pdf_path(),
            '-o', self.get_pdf_path()
        ))).assert_(stderr=re('Input file is the same as output file'), rc=1)
        with open(pdf_path, 'rb') as pdf_file:
            pdf_after = pdf_file.read()
        assert_equal(pdf_before, pdf_after)

# vim:ts=4 sts=4 sw=4 et
