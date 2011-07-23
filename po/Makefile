# Copyright © 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = ..
include $(srcdir)/Makefile.common

PO_FILES = $(wildcard *.po)
MO_FILES = $(PO_FILES:.po=.mo)
SOURCE_FILES = $(wildcard $(srcdir)/*.cc $(srcdir)/*.hh)

XGETTEXT_OPTIONS = \
	--no-location \
	--language=C++ \
	--keyword=_ --keyword=N_ \
	--package-name=$(PACKAGE_NAME) \
	--package-version=$(PACKAGE_VERSION) \
	--msgid-bugs-address=$(PACKAGE_BUGREPORT)

.PHONY: all
all: $(PACKAGE_NAME).pot $(PO_FILES) $(MO_FILES)

$(PACKAGE_NAME).pot: $(XGETTEXT_HELPER) $(SOURCE_FILES)
	$(XGETTEXT) $(XGETTEXT_OPTIONS) -D $(srcdir) $(notdir $(SOURCE_FILES)) > $(@)

%.po: $(PACKAGE_NAME).pot
	$(MSGMERGE) -o $(@) $(@) $(<)

%.mo: %.po
	$(MSGFMT) -o $(@) -c $(<)

.PHONY: clean
clean:
	rm -f *.mo $(PACKAGE_NAME).pot

# vim:ts=4 sw=4 noet
