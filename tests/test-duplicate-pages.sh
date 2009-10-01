# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://code.google.com/p/pdf2djvu/issues/detail?id=23
# Fixed in [ede3c622526a].

. ./common.sh

! test_pdf2djvu --pages=1,1 && \
grep '^Duplicate page identifier:' "$error_log" > /dev/null
! test_pdf2djvu_indirect --pages=1,1 && \
grep '^Duplicate page identifier:' "$error_log" > /dev/null
