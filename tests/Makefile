# Copyright © 2009, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

pdf2djvu = ../pdf2djvu
export pdf2djvu

test_scripts = $(wildcard test-*.sh)
tests = $(test_scripts:.sh=)

stamp_files = $(tests:%=%.stamp)
pdf_files = $(tests:%=%.pdf)
djvu_files = $(tests:%=%.djvu)

in_files = $(wildcard *.in)

generated_files = $(in_files:.in=) $(stamp_files) $(pdf_files)

.PHONY: all
all: $(generated_files)

.PHONY: prepare
prepare: $(in_files:.in=) $(pdf_files)
	rm -f *.log *.pyc

.PHONY: clean
clean:
	rm -f $(generated_files) *.log *.djvu *.pyc

%.stamp: %.sh %.pdf $(pdf2djvu)
	sh -e -x $(<)
	touch $(@)

%.pdf: %.tex
	pdftex $(<)

%: %.in
	./$(<)

../pdf2djvu:
	$(MAKE) -C ../

# vim:ts=4 sw=4 noet
