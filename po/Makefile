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

srcdir = ..
include $(srcdir)/autoconf.mk

po_files = $(wildcard *.po)
mo_files = $(po_files:.po=.mo)
source_files = $(wildcard $(srcdir)/*.cc $(srcdir)/*.hh)

gettext-common-options = \
	--add-location=file \
	--sort-output \
	--no-wrap \

xgettext = $(XGETTEXT) \
	$(gettext-common-options) \
	--from-code=UTF-8 \
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
all: pdf2djvu.pot $(po_files) $(mo_files)

pdf2djvu.pot: $(XGETTEXT_HELPER) $(source_files)
	$(xgettext) -D $(srcdir) $(sort $(notdir $(source_files))) > $(@)

%.po: pdf2djvu.pot
	$(msgmerge) $(@) $(<)
	touch $(@)

%.mo: %.po
	$(msgfmt) -o $(@) -c $(<)

.PHONY: check
check: $(po_files)
	$(srcdir)/private/check-po $(^)

.PHONY: clean
clean:
	rm -f *.mo pdf2djvu.pot

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
