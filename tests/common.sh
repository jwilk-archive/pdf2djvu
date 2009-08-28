# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

gm=`command -v gm`

eval_image_size()
{
  eval `$gm identify -format 'width=%w; height=%h;' "$1" | sed -e 's/[.][0-9]\+//g' -e 's/[A-Z][A-Za-z]*//' | tr -dc '0-9a-z=;'`
}

pdf2djvu=`command -v ${pdf2djvu:-pdf2djvu}`

pdf2djvu()
{
    "$pdf2djvu" "$@"
}

test_pdf2djvu()
{
    pdf_file="${0%*.sh}".pdf
    djvu_file="${0%*.sh}".djvu
    pdf2djvu -q "$pdf_file" -o "$djvu_file" "$@"
}

# vim:ts=4 sw=4 et
