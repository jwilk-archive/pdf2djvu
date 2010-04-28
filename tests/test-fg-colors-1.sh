# Copyright © 2010 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# http://code.google.com/p/pdf2djvu/issues/detail?id=47
# Fixed in [3d0f55ae5a65]

# http://code.google.com/p/pdf2djvu/issues/detail?id=45
# Fixed in [fc7df6c4d3d3]

. ./common.sh

for settings in default web black 1 2 4 255 256 652
do
    test_pdf2djvu "--fg-colors=$settings"
    ddjvu "$djvu_file" > /dev/null
done

# vim:ts=4 sw=4 et
