# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [9a9650e7f06f].

. ./common.sh

test_pdf2djvu
djvudump "$djvu_file" | grep 'BG44 \[[0-9][0-9]\] .* 75x75' > /dev/null
