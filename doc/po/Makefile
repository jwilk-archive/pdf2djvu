# Copyright © 2009-2018 Jakub Wilk <jwilk@jwilk.net>
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

srcdir = ../..
include $(srcdir)/autoconf.mk

po4a-common-options = \
	--format=docbook

po4a-getext-options = \
	$(po4a-common-options) \
	--master-charset=UTF-8 \
	--package-name=pdf2djvu \
	--package-version=$(PACKAGE_VERSION) \
	--msgid-bugs-address=$(PACKAGE_BUGREPORT) \
	--copyright-holder="Jakub Wilk <jwilk@jwilk.net>"

msgmerge_opts = \
	--no-location \
	--no-wrap \

ifneq "$(wildcard $(srcdir)/.hg/)" ""
msgmerge_opts += --backup=off
endif

po4a-update-po = po4a-updatepo \
	$(po4a-getext-options) \
	--msgmerge-opt="$(msgmerge_opts)"

po4a-gettextize = po4a-gettextize \
	$(po4a-getext-options)

po4a-translate = po4a-translate \
	$(po4a-common-options)

source = ../manpage.xml
po_files = $(wildcard *.po)
langs = $(po_files:.po=)
xml_files = $(addsuffix .xml,$(langs))
man_files = $(addsuffix .1,$(addprefix pdf2djvu.,$(langs)))

.PHONY: all
all: $(man_files)

pdf2djvu.%.1: %.xml
	! [ -s $(<) ] || $(srcdir)/tools/build-manpage -l $(<) $(@)

.PHONY: xml
xml: $(xml_files)

manpage.pot: $(source)
	$(po4a-gettextize) -m $(<) -p $(@)
	perl -pi -e 's/^([#][.])\s*(\S.*\S)\s*$$/$$1 $$2\n/' $(@)

$(po_files): %.po: $(source)
	$(po4a-update-po) -m $(<) -p $(@)
	perl -pi -e 's/^([#][.])\s*(\S.*\S)\s*$$/$$1 $$2\n/' $(@)
	touch $(@)

%.xml: %.po
	$(po4a-translate) -k 100 -m $(source) -p $(<) -l $(@)

.PHONY: check
check: $(po_files)
	$(srcdir)/private/check-po $(^)

.PHONY: clean
clean:
	rm -f *.1 *.xml *.pot

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
