# Copyright © 2009-2015 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdfdjvu.
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

export LC_ALL = C

export pdf2djvu = ../pdf2djvu$(EXEEXT)

test_scripts = $(wildcard test-*.py)
tests = $(test_scripts:.py=)

tex_files = $(wildcard test-*.tex) $(wildcard test-*.xetex)
pdf_files = $(tex_files:%.tex=%.pdf)

in_files = $(wildcard *.in)

generated_files = $(in_files:.in=) $(pdf_files)

.PHONY: all
ifeq "$(origin pdf2djvu)" "file"
all: $(pdf2djvu)
endif
all: $(generated_files)
	nosetests -v

.PHONY: prepare
prepare: $(generated_files)
	rm -f *.pyc

.PHONY: clean
clean:
	rm -f *.djvu *.pyc

.PHONY: vcs-clean
vcs-clean: clean
	rm -f $(generated_files)

%.pdf: %.tex
	pdftex $(<)
	rm -f $(<:.tex=.log)

%.pdf: %.xetex
	xetex --output-driver="xdvipdfmx -z0" $(<)
	rm -f $(<:.xetex=.log)

%: %.in
	./$(<)

../pdf2djvu$(EXEEXT):
	$(MAKE) -C ../

# vim:ts=4 sts=4 sw=4 noet
