# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [a244b65e0661], [2e5ab20f8a07].

. common.sh

test_pdf2djvu
djvused -e 'select 1; print-ant' "$djvu_file" | grep '^(maparea "#p0002.djvu" "" (rect [0-9]\+ [0-9]\+ [0-9]\+ [0-9]\+) (xor))$' > /dev/null
djvused -e 'select 2; print-ant' "$djvu_file" | grep '^(maparea "#p0001.djvu" "" (rect [0-9]\+ [0-9]\+ [0-9]\+ [0-9]\+) (border #ff7f00))$' > /dev/null
