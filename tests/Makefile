# Copyright © 2009-2012 Jakub Wilk
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

export LC_ALL = C

pdf2djvu = ../pdf2djvu
export pdf2djvu

all_test_scripts = $(wildcard test-*.py)
skipped_test_scripts = $(wildcard $(addprefix test-,$(addsuffix .py,$(skip))))
test_scripts = $(filter-out $(skipped_test_scripts),$(all_test_scripts))
tests = $(test_scripts:.py=)

pdf_files = $(tests:%=%.pdf)

in_files = $(wildcard *.in)

generated_files = $(in_files:.in=) $(pdf_files)

.PHONY: all
all: $(generated_files) $(pdf2djvu)
	nosetests -v $(test_scripts)

.PHONY: prepare
prepare: $(in_files:.in=) $(pdf_files)
	rm -f *.pyc

.PHONY: clean
clean:
	rm -f $(generated_files) *.djvu *.pyc

%.pdf: %.tex
	pdftex $(<)
	rm -f $(<:.tex=.log)

%: %.in
	./$(<)

../pdf2djvu:
	$(MAKE) -C ../

# vim:ts=4 sts=4 sw=4 noet
