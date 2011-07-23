# Copyright © 2009, 2010, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = ../..
include $(srcdir)/Makefile.common
include $(srcdir)/doc/Makefile.common

XSLTPROC += --param man.output.lang.in.name.enabled 1

po4a-common-options = \
	--format=docbook

po4a-update-po = po4a-updatepo \
	$(po4a-common-options) \
	--package-name=pdf2djvu \
	--package-version=$(PACKAGE_VERSION) \
	--msgid-bugs-address=$(PACKAGE_BUGREPORT) \
	--copyright-holder="Jakub Wilk" \
	--msgmerge-opt="--no-location"

po4a-translate = po4a-translate \
	$(po4a-common-options)

po_files = $(wildcard pdf2djvu.[a-z][a-z].po)
xml_files = $(po_files:.po=.xml)
man_files = $(po_files:.po=.1)

.PHONY: all
all: $(man_files)

.PHONY: xml
xml: $(xml_files)

$(po_files): %.po: pdf2djvu.xml
	$(po4a-update-po) -m $(<) -p $(@)
	rm -f $(@)~
	touch $(@)

%.xml: %.po
ifneq "$(quality-check)" ""
	! msgattrib --only-fuzzy $(<) | grep .
	! msgattrib --untranslated $(<) | grep .
endif
	$(po4a-translate) -m pdf2djvu.xml -p $(<) -l $(@)
	sed -i -e 's/<\(refentry\)>/<\1 lang="$(subst .,,$(suffix $(basename $(@))))">/' $(@)

.PHONY: clean
clean:
	rm -f $(addprefix pdf2djvu.,[a-z][a-z].1 [a-z][a-z].xml pot) *~

# vim:ts=4 sw=4 noet
