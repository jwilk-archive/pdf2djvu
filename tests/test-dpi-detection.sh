# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://sf.net/tracker/?func=detail&aid=1972089&group_id=32953&atid=406583
# Fixed in [5e3937296514].

. ./common.sh

test_pdf2djvu --guess-dpi
djvudump "$djvu_file" | grep 'INFO .* DjVu 100x200,' > /dev/null
