# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://bugs.debian.org/508391
# Fixed in [f5d4727b2490].

. ./common.sh

test_pdf2djvu
! djvused -e print-outline "$djvu_file" | grep .
