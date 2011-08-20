# Copyright © 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://code.google.com/p/pdf2djvu/issues/detail?id=59
# Fixed in [6c155e7cc346]

. ./common.sh

test_pdf2djvu --monochrome
djvudump "$djvu_file" | grep -E 'Sjbz \[[0-9]{4,}\]' >/dev/null
