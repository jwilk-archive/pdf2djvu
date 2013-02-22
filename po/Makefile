# Copyright © 2009, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = ..
include $(srcdir)/Makefile.common

PO_FILES = $(wildcard *.po)
MO_FILES = $(PO_FILES:.po=.mo)
SOURCE_FILES = $(wildcard $(srcdir)/*.cc $(srcdir)/*.hh)

gettext-common-options = \
	--no-location \
	--sort-output \
	--no-wrap \

xgettext = $(XGETTEXT) \
	$(gettext-common-options) \
	--language=C++ \
	--keyword=_ --keyword=N_ \
	--package-name=pdf2djvu \
	--package-version=$(PACKAGE_VERSION) \
	--msgid-bugs-address=$(PACKAGE_BUGREPORT) \
	--add-comments=L10N: \

msgmerge = $(MSGMERGE) \
	$(gettext-common-options) \
	--verbose \

msgfmt = $(MSGFMT) \
	--check \
	--verbose \

.PHONY: all
all: pdf2djvu.pot $(PO_FILES) $(MO_FILES)

pdf2djvu.pot: $(XGETTEXT_HELPER) $(SOURCE_FILES)
	$(xgettext) -D $(srcdir) $(notdir $(SOURCE_FILES)) > $(@)

%.po: pdf2djvu.pot
	$(msgmerge) -o $(@) $(@) $(<)

%.mo: %.po
ifneq "$(quality-check)" ""
	! msgattrib --only-fuzzy $(<) | grep .
	! msgattrib --untranslated $(<) | grep .
endif
	$(msgfmt) -o $(@) -c $(<)

.PHONY: clean
clean:
	rm -f *.mo pdf2djvu.pot

# vim:ts=4 sw=4 noet
