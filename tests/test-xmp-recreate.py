# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    case,
    assert_true,
    assert_grep,
    assert_well_formed_xml,
)

class test(case):
    '''
    fixed in [d8c60ea6bc37]
    '''
    def test_verbatim(self):
        self.pdf2djvu('--verbatim-metadata').assert_()
        xmp = self.extract_xmp()
        assert_true(xmp is None)

    def test_no_verbatim(self):
        self.require_feature('GNOME XSLT')
        self.pdf2djvu().assert_()
        xmp = self.extract_xmp()
        assert_well_formed_xml(xmp)
        assert_grep(xmp, '>image/vnd.djvu<')

# vim:ts=4 sts=4 sw=4 et
