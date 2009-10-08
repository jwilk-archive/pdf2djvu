# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

pdf2djvu = ../pdf2djvu
export pdf2djvu

TEST_SCRIPTS = $(wildcard test-*.sh)
TESTS = $(TEST_SCRIPTS:.sh=)

STAMP_FILES = $(TESTS:%=%.stamp)
PDF_FILES = $(TESTS:%=%.pdf)
DJVU_FILES = $(TESTS:%=%.djvu)

IN_FILES = $(wildcard *.in)

GENERATED_FILES = $(IN_FILES:.in=) $(STAMP_FILES) $(PDF_FILES)

.PHONY: all
all: $(GENERATED_FILES)

.PHONY: prepare
prepare: $(IN_FILES:.in=) $(PDF_FILES)
	rm -f *.log *.pyc

.PHONY: clean
clean:
	rm -f $(GENERATED_FILES) *.log *.djvu *.pyc

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
