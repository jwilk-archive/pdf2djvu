# Copyright © 2009-2013 Jakub Wilk <jwilk@jwilk.net>
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

srcdir = ..
include $(srcdir)/autoconf.mk

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
	--update \
	--verbose \

ifneq "$(wildcard $(srcdir)/.hg/)" ""
msgmerge += --backup=off
endif

msgfmt = $(MSGFMT) \
	--check \
	--verbose \

.PHONY: all
all: pdf2djvu.pot $(PO_FILES) $(MO_FILES)

pdf2djvu.pot: $(XGETTEXT_HELPER) $(SOURCE_FILES)
	$(xgettext) -D $(srcdir) $(notdir $(SOURCE_FILES)) > $(@)

%.po: pdf2djvu.pot
	$(msgmerge) $(@) $(<)

%.mo: %.po
	$(msgfmt) -o $(@) -c $(<)

GREP_COLOR = grep --color=auto

.PHONY: check
check: $(MO_FILES)
	ls $(PO_FILES) | xargs -t -L1 -I{} msgfmt --check -o /dev/null {}
	! ls $(PO_FILES) | xargs -t -L1 -I{} msgattrib --untranslated {} | $(GREP_COLOR) '.*'
	! ls $(PO_FILES) | xargs -t -L1 -I{} msgattrib --only-fuzzy {} | $(GREP_COLOR) '.*'

.PHONY: clean
clean:
	rm -f *.mo pdf2djvu.pot

# vim:ts=4 sts=4 sw=4 noet
