# encoding=UTF-8

# Copyright © 2009, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

from common import (
    case,
    assert_grep,
)

class test(case):
    def test_verbatim(self):
        self.pdf2djvu('--verbatim-metadata').assert_()
        xmp = self.extract_xmp()
        assert_grep(xmp, '<broken')

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
        assert_grep(xmp, '<broken')

# vim:ts=4 sts=4 sw=4 et
