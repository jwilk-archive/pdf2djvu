# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

. ./common.sh

test_pdf2djvu --verbatim-metadata
extract_xmp --raw "$djvu_file" | grep -F '<broken' > /dev/null
test_pdf2djvu
extract_xmp --raw "$djvu_file" | grep -F '<broken' > /dev/null
