# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://code.google.com/p/pdf2djvu/issues/detail?id=20
# Fixed in [c06a41afdc46].

. ./common.sh

test_pdf2djvu
djvused -e print-pure-txt "$djvu_file" | grep '^Lorem ipsum *$' > /dev/null
test_pdf2djvu --crop-text
djvused -e print-pure-txt "$djvu_file" | grep '^Lorem *$' > /dev/null
