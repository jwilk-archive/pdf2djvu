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

srcdir = ../..
include $(srcdir)/autoconf.mk

include $(srcdir)/doc/Makefile.common

XSLTPROC += --param man.output.lang.in.name.enabled 1

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

po_files = $(wildcard pdf2djvu.[a-z][a-z].po)
xml_files = $(po_files:.po=.xml)
man_files = $(po_files:.po=.1)

.PHONY: all
all: $(man_files)

.PHONY: xml
xml: $(xml_files)

pdf2djvu.pot: pdf2djvu.xml
	$(po4a-gettextize) -m $(<) -p $(@)

$(po_files): %.po: pdf2djvu.xml
	$(po4a-update-po) -m $(<) -p $(@)
	touch $(@)

%.xml: %.po
	$(po4a-translate) -m pdf2djvu.xml -p $(<) -l $(@)

GREP_COLOR = grep --color=auto

.PHONY: check
check: $(MO_FILES)
	ls $(po_files) | xargs -t -L1 -I{} msgfmt --check -o /dev/null {}
	! ls $(po_files) | xargs -t -L1 -I{} msgattrib --untranslated {} | $(GREP_COLOR) '.*'
	! ls $(po_files) | xargs -t -L1 -I{} msgattrib --only-fuzzy {} |  $(GREP_COLOR) '.*'

.PHONY: clean
clean:
	rm -f $(addprefix pdf2djvu.,[a-z][a-z].1 [a-z][a-z].xml pot) *~

# vim:ts=4 sts=4 sw=4 noet
