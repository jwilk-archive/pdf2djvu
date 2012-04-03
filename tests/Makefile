# Copyright © 2009, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

# To test everything, run:
#
#    make
#
# You can use the ‘skip’ variable to skip some tests. E.g. to skip tests
# requiring GraphicsMagick, run:
#
#    make skip='*+gm'

pdf2djvu = ../pdf2djvu
export pdf2djvu

all_test_scripts = $(wildcard test-*.sh)
skipped_test_scripts = $(wildcard $(addprefix test-,$(addsuffix .sh,$(skip))))
test_scripts = $(filter-out $(skipped_test_scripts),$(all_test_scripts))
tests = $(test_scripts:.sh=)

stamp_files = $(tests:%=%.stamp)
pdf_files = $(tests:%=%.pdf)

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
