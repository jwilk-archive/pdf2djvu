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

import os

from tools import (
    assert_equal,
    case,
    re,
)

here = os.path.dirname(__file__)
srcdir = os.path.join(here, os.pardir)

class test(case):

    def __init__(self):
        path = os.path.join(srcdir, 'doc', 'changelog')
        with open(path) as file:
            line = file.readline()
        self.changelog_version = line.split()[1].strip('()')

    def test_source(self):
        path = os.path.join(srcdir, 'configure.ac')
        with open(path) as file:
            for line in file:
                match = re(r'^AC_INIT[(]\[pdf2djvu\], \[([0-9.]+)\]').match(line)
                if match is not None:
                    break
        src_version = match.group(1)
        assert_equal(src_version, self.changelog_version)

    def test_executable(self):
        r = self.pdf2djvu('--version')
        r.assert_(stderr=re('^pdf2djvu [0-9.]+$', re.M), rc=0)
        exec_version = r.stderr.splitlines()[0]
        _, exec_version = exec_version.split()
        assert_equal(exec_version, self.changelog_version)

# vim:ts=4 sts=4 sw=4 et
