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

from common import (
    assert_regexp_matches,
    case,
)

class test(case):
    def test_verbatim(self):
        self.pdf2djvu('--verbatim-metadata').assert_()
        xmp = self.extract_xmp()
        assert_regexp_matches(xmp, '<broken')

    def test_no_verbatim(self):
        self.require_feature('GNOME XSLT')
        r = self.pdf2djvu()
        r.assert_(stderr='''\
Entity: line 1: parser error : error parsing attribute name
<x:xmpmeta xmlns:x="adobe:ns:meta/"> <broken </x:xmpmeta>
                                             ^
Entity: line 1: parser error : attributes construct error
<x:xmpmeta xmlns:x="adobe:ns:meta/"> <broken </x:xmpmeta>
                                             ^
Entity: line 1: parser error : Couldn't find end of Start Tag broken line 1
<x:xmpmeta xmlns:x="adobe:ns:meta/"> <broken </x:xmpmeta>
                                             ^
''')
        xmp = self.extract_xmp()
        assert_regexp_matches(xmp, '<broken')

# vim:ts=4 sts=4 sw=4 et
