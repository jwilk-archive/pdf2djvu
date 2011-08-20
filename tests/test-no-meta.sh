# Copyright © 2009, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [d1b09854a9bc]

. ./common.sh

test_pdf2djvu --no-metadata --dpi=72 &&
! djvused -e print-meta "$djvu_file" | grep . > /dev/null
