# Copyright © 2007-2018 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdf2djvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

srcdir = ..

.PHONY: all
all: pdf2djvu.1

pdf2djvu.1: manpage.xml
	$(srcdir)/tools/build-manpage $(<) $(@)

.PHONY: check
check: check-docbook

.PHONY: check-docbook
check-docbook: manpage.xml
	 xmllint --nonet --noout --valid $(<)

.PHONY: clean
clean:
	rm -f *.1

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
