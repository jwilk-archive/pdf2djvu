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
    assert_regex,
    assert_well_formed_xml,
    case,
)

class test(case):
    '''
    https://bitbucket.org/jwilk/pdf2djvu/issue/12
    fixed in [490a08283fb4]
    '''
    def test_verbatim(self):
        self.pdf2djvu('--verbatim-metadata').assert_()
        xmp = self.extract_xmp()
        assert_well_formed_xml(xmp)
        assert_regex(xmp, '>application/pdf<')

    def test_no_verbatim(self):
        self.require_feature('GNOME XSLT')
        self.pdf2djvu().assert_()
        xmp = self.extract_xmp()
        assert_well_formed_xml(xmp)
        assert_regex(xmp, '>image/vnd.djvu<')

# vim:ts=4 sts=4 sw=4 et
