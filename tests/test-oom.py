# encoding=UTF-8

# Copyright © 2016 Jakub Wilk <jwilk@jwilk.net>
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
import resource

from tools import (
    case,
    re,
)

@contextlib.contextmanager
def vm_limit(limit):
    [lim_soft, lim_hard] = resource.getrlimit(resource.RLIMIT_AS)
    if lim_hard != resource.RLIM_INFINITY and lim_hard < limit:
        limit = lim_hard
    resource.setrlimit(resource.RLIMIT_AS, (limit, lim_hard))
    try:
        yield
    finally:
        resource.setrlimit(resource.RLIMIT_AS, (lim_soft, lim_hard))

class test(case):
    # Bug: https://bitbucket.org/jwilk/pdf2djvu/issue/107
    # + fixed in 0.9.4 [87708193290f]

    def test(self):
        # Before Poppler 0.24, the Splash backend would just segfault on OOM.
        # https://cgit.freedesktop.org/poppler/poppler/commit/?id=e04287f2682e
        self.require_poppler(0, 24)
        [lim_soft, lim_hard] = resource.getrlimit(resource.RLIMIT_AS)
        with vm_limit(1 << 30):  # 1 GiB virtual memory limit
            r = self.pdf2djvu()
        r.assert_(stderr=re('Out of memory\n'), rc=1)

# vim:ts=4 sts=4 sw=4 et
