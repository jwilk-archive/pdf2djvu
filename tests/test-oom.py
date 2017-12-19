# encoding=UTF-8

# Copyright © 2016-2017 Jakub Wilk <jwilk@jwilk.net>
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

import contextlib
import re
import resource
import string
import sys

from tools import (
    SkipTest,
    case,
)

@contextlib.contextmanager
def vm_limit(limit):
    code = 'import resource\nfor n in resource.getrlimit(resource.RLIMIT_AS):\n print(n)'
    try:
        RLIMIT_AS = resource.RLIMIT_AS
    except AttributeError:
        # OpenBSD doesn't have RLIMIT_AS;
        # RLIMIT_DATA is good enough.
        RLIMIT_AS = resource.RLIMIT_DATA
        code = re.sub(r'_AS\b', '_DATA', code)
    [lim_soft, lim_hard] = resource.getrlimit(RLIMIT_AS)
    if lim_hard != resource.RLIM_INFINITY and lim_hard < limit:
        limit = lim_hard
    resource.setrlimit(RLIMIT_AS, (limit, lim_hard))
    try:
        r = case().run(sys.executable, '-c', code)
        r.assert_(stdout=re.compile(''))
        (cld_soft_lim, cld_hard_lim) = map(int, r.stdout.splitlines())
        if cld_soft_lim != limit or cld_hard_lim != lim_hard:
            message = 'virtual memory limit did not propagate to subprocess'
            if sys.platform.rstrip(string.digits) == 'gnu':
                raise SkipTest(message + ': https://savannah.gnu.org/bugs/?43320')
            raise RuntimeError(message)
        yield
    finally:
        resource.setrlimit(RLIMIT_AS, (lim_soft, lim_hard))

class test(case):
    # Bug: https://github.com/jwilk/pdf2djvu/issues/107
    # + fixed in 0.9.4 [fb1dcf60c77dfc39de80d60288449caf767d8b54]

    def test(self):
        with vm_limit(1 << 30):  # 1 GiB virtual memory limit
            r = self.pdf2djvu()
        r.assert_(stderr=re.compile(
            'Out of memory\n'
            '|AddressSanitizer failed to allocate '
        ), rc=1)

# vim:ts=4 sts=4 sw=4 et
