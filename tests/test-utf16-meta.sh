# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# Fixed in [90111a528eb5].

. ./common.sh

title='\316\232\317\211\316\264\316\271\316\272\316\277\317\200\316\277\316\257\316\267\317\203\316\267 \317\207\316\261\317\201\316\261\316\272\317\204\316\256\317\201\317\211\316\275'
test_pdf2djvu --dpi=72
djvused -e print-meta "$djvu_file" | grep -F 'Title	"'"$title"'"' > /dev/null
