# Copyright © 2009-2019 Jakub Wilk <jwilk@jwilk.net>
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
include $(srcdir)/autoconf.mk

PYTHON = python

export pdf2djvu = ../pdf2djvu$(EXEEXT)

test_scripts = $(wildcard test-*.py)
tests = $(test_scripts:.py=)

tex_files = $(wildcard test-*.tex)
pdf_files = $(addsuffix .pdf,$(basename $(tex_files)))

in_files = $(wildcard *.in)

generated_files = $(in_files:.in=) $(pdf_files)

noseargs =

.PHONY: all
ifeq "$(origin pdf2djvu)" "file"
all: $(pdf2djvu)
endif
all: $(generated_files)
	$(PYTHON) -c 'import nose; nose.main()' -v $(noseargs)

.PHONY: prepare
prepare: $(generated_files)
	rm -f *.py[co]

.PHONY: clean
clean:
	rm -f *.djvu *.py[co]

.PHONY: vcs-clean
vcs-clean: clean
	rm -f $(generated_files)

%.pdf: %.tex
	luatex $(<)
	rm -f $(<:.tex=.log)

%.pdf: %.pdf.in
	./$(<)

%: %.in
	./$(<)

../pdf2djvu$(EXEEXT):
	$(MAKE) -C ../

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
