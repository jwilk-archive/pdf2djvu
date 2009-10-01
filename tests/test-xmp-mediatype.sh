# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://code.google.com/p/pdf2djvu/issues/detail?id=12
# Fixed in [490a08283fb4].

. ./common.sh

test_pdf2djvu --verbatim-metadata
extract_xmp "$djvu_file" | grep -F '>application/pdf<' > /dev/null
test_pdf2djvu
extract_xmp "$djvu_file" | grep -F '>image/vnd.djvu<' > /dev/null
