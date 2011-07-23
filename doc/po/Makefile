# Copyright © 2009, 2010, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = ../..
include $(srcdir)/Makefile.common
include $(srcdir)/doc/Makefile.common

XSLTPROC += --param man.output.lang.in.name.enabled 1

PO4A_OPTIONS = \
	--rm-backups \
	--package-name=$(PACKAGE_NAME) \
	--package-version=$(PACKAGE_VERSION) \
	--msgid-bugs-address=$(PACKAGE_BUGREPORT) \
	--copyright-holder="Jakub Wilk"

PO_FILES = $(wildcard *.po)
XML_FILES = $(PO_FILES:.po=.xml)
MAN_FILES = $(PO_FILES:.po=.1)

.PHONY: all
all: $(MAN_FILES)

.PHONY: xml
xml: $(XML_FILES)

%.xml: %.po
	po4a $(PO4A_OPTIONS) po4a.conf
	sed -i -e '/^#:/ d' *.po
	sed -i -e 's/<\(refentry\)>/<\1 lang="$(subst .,,$(suffix $(basename $(@))))">/' $(@)

# vim:ts=4 sw=4 noet
