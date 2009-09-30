# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

pdf2djvu=`command -v ${pdf2djvu:-pdf2djvu}`

pdf2djvu()
{
    "$pdf2djvu" "$@"
}

_pdf2djvu_variables()
{
    pdf_file="${0%*.sh}".pdf
    djvu_file="${0%*.sh}".djvu
    error_log="${0%.sh}".pdf.log
}

_test_pdf2djvu()
{
    "$pdf2djvu" -q "$pdf_file" "$@" 2> "$error_log"
}

test_pdf2djvu()
{
    _pdf2djvu_variables
    _test_pdf2djvu -o "$djvu_file" "$@"
}

test_pdf2djvu_indirect()
{
    _pdf2djvu_variables
    _test_pdf2djvu -i "$djvu_file" "$@"
}

extract_xmp()
{
    local raw
    if [ "$1" = '--raw' ]
    then
        raw=1
        shift
    else
        raw=0
    fi
    djvused -e output-ant "$@" \
    | sed -n -e '/^(xmp \(.*\))$/ { s//printf \1/ p; q }' \
    | sh \
    | ( grep '.*' || printf '<pdf2djvu:empty xmlns:pdf2djvu="http://pdf2djvu.googlecode.com/"/>') \
    | ( [ "$raw" -eq 0 ] && xmllint -format - || cat )
}

# vim:ts=4 sw=4 et
