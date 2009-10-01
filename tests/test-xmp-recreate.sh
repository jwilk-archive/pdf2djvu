# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [d8c60ea6bc37].

. ./common.sh

test_pdf2djvu --verbatim-metadata
extract_xmp "$djvu_file" | grep -F '<pdf2djvu:empty' > /dev/null
test_pdf2djvu
extract_xmp "$djvu_file" | grep -F '>image/vnd.djvu<' > /dev/null
