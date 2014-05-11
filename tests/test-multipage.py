# encoding=UTF-8

# Copyright © 2010, 2012 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.


from common import (
    case,
)

class test(case):
    '''
    https://code.google.com/p/pdf2djvu/issues/detail?id=50
    '''
    def test(self):
        self.pdf2djvu('--pages=2').assert_()

# vim:ts=4 sw=4 et
