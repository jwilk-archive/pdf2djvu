# Copyright © 2007-2015 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = .
include $(srcdir)/Makefile.common

exe = pdf2djvu$(EXEEXT)

.PHONY: all
all: $(exe)

ifneq "$(WINDRES)" ""
win32-version.o: win32-version.rc autoconf.hh
	$(WINDRES) -c 65001 -o $(@) $(<)
$(exe): win32-version.o
endif

include Makefile.dep

%.hh: %.xml
	tools/xml2c < $(<) > $(@)

paths.hh: tools/generate-paths-hh Makefile.common
	$(<) $(foreach var,localedir djvulibre_bindir,$(var) $($(var)))

$(exe): config.o
$(exe): debug.o
$(exe): djvu-outline.o
$(exe): i18n.o
$(exe): image-filter.o
$(exe): pdf-backend.o
$(exe): pdf-dpi.o
$(exe): pdf2djvu.o
$(exe): sexpr.o
$(exe): string-format.o
$(exe): system.o
$(exe): version.o
$(exe): xmp.o
$(exe):
	$(LINK.cc) $(^) $(LDLIBS) -o $(@)

XML_FILES = $(wildcard *.xml)
XML_HH_FILES = $(XML_FILES:.xml=.hh)

.PHONY: clean
clean:
	rm -f $(exe) *.o paths.hh $(XML_HH_FILES)

.PHONY: distclean
distclean: clean
	rm -f autoconf.hh Makefile.common config.status config.log

.PHONY: test
test: $(exe)
	$(MAKE) -C tests/

MAN_PAGES = $(wildcard doc/*.1 doc/po/*.1)
MO_FILES = $(wildcard po/*.mo)

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) $(exe) $(DESTDIR)$(bindir)
ifneq ($(MAN_PAGES),)
	set -e; for manpage in $(MAN_PAGES); \
	do \
		set -x; \
		basename=`basename $$manpage`; \
		suffix="$${basename#*.}"; \
		locale="$${suffix%.*}"; \
		[ $$locale = $$suffix ] && locale=; \
		$(INSTALL) -d $(DESTDIR)$(mandir)/$$locale/man1/; \
		$(INSTALL) -m 644 $$manpage $(DESTDIR)$(mandir)/$$locale/man1/"$${basename%%.*}.$${basename##*.}"; \
	done
endif
ifneq ($(MO_FILES),)
	set -e; for locale in $(basename $(notdir $(MO_FILES))); \
	do \
		$(INSTALL) -d $(DESTDIR)$(localedir)/$$locale/LC_MESSAGES/; \
		$(INSTALL) -m 644 po/$$locale.mo $(DESTDIR)$(localedir)/$$locale/LC_MESSAGES/pdf2djvu.mo; \
	done
endif

# vim:ts=4 sts=4 sw=4 noet
