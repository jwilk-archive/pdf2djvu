# encoding=UTF-8

# Copyright © 2022 Jakub Wilk <jwilk@jwilk.net>
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

import re

from tools import (
    case,
)

class test(case):

    # Make sure poppler-data files can be loaded.

    # FIXME: This does not really work when testing under Wine:
    #
    #   test-ja.test.test ... FAIL
    #
    #   ======================================================================
    #   FAIL: test-ja.test.test
    #   ----------------------------------------------------------------------
    #   Traceback (most recent call last):
    #     File "/usr/lib/python2.7/dist-packages/nose/case.py", line 197, in runTest
    #       self.test(*self.arg)
    #     File ".../pdf2djvu/tests/test-ja.py", line 27, in test
    #       self.pdf2djvu().assert_()
    #     File ".../pdf2djvu/tests/tools.py", line 98, in assert_
    #       assert_multi_line_equal(self.stderr, stderr)
    #   AssertionError: "PDF syntax error: Couldn't find a font for 'System'\nPDF syntax error: [truncated]... != ''
    #   - PDF syntax error: Couldn't find a font for 'System'
    #   - PDF syntax error: Couldn't find a font for 'System'
    #
    #   -------------------- >> begin captured stdout << ---------------------
    #   $ sh -c 'wine ../win32/dist/pdf2djvu.exe "$@"' sh -q .../tests/test-ja.pdf -o .../tests/test-ja.djvu
    #
    #   --------------------- >> end captured stdout << ----------------------
    #
    # This is likely because:
    #
    # * Wine fonts are not present in the Windows system font directory (C:\Windows\Fonts).
    # * Fontconfig looks for system fonts only in that directory:
    #   https://gitlab.freedesktop.org/fontconfig/fontconfig/issues/32
    # * Poppler doesn't seem to like using a PostScript font as fallback.
    #
    # Work-around:
    #
    #   cp /usr/share/wine/fonts/system.ttf $WINEPREFIX/drive_c/windows/Fonts/

    def test(self):
        self.pdf2djvu().assert_()
        r = self.print_text()
        r.assert_(stdout=re.compile(r'^!\s*\n'))

# vim:ts=4 sts=4 sw=4 et
