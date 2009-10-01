# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [18b2ae04de2f], [fa7d4addf18e].

. ./common.sh

test_pdf2djvu
djvused -e print-pure-txt "$djvu_file" | grep '^fluorografia *$' > /dev/null
test_pdf2djvu --no-nfkc
djvused -e print-pure-txt "$djvu_file" | grep '^ﬂuorograﬁa *$' > /dev/null
