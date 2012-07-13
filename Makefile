# Copyright © 2007, 2008, 2009 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

srcdir = .
include $(srcdir)/Makefile.common

.PHONY: all
all: pdf2djvu

ifneq "$(WINDRES)" ""
version.o: version.rc version.hh
	$(WINDRES) -c 1252 -o $(@) $(<)
pdf2djvu: version.o
endif

include Makefile.dep

%.hh: %.xml
	tools/xml2c < $(<) > $(@)

paths.hh: tools/generate-paths-hh Makefile.common
	$(<) $(foreach var,localedir djvulibre_bindir,$(var) $($(var)))

pdf2djvu: config.o
pdf2djvu: debug.o
pdf2djvu: i18n.o
pdf2djvu: image-filter.o
pdf2djvu: pdf-backend.o
pdf2djvu: pdf-dpi.o
pdf2djvu: pdf2djvu.o
pdf2djvu: sexpr.o
pdf2djvu: string-format.o
pdf2djvu: system.o
pdf2djvu: xmp.o
pdf2djvu:
	$(LINK.cc) $(^) $(LDLIBS) -o $(@)

XML_FILES = $(wildcard *.xml)
XML_HH_FILES = $(XML_FILES:.xml=.hh)

.PHONY: clean
clean:
	rm -f pdf2djvu *.o paths.hh $(XML_HH_FILES)

.PHONY: distclean
distclean: clean
	rm -f version.hh Makefile.common config.status config.log

.PHONY: test
test: pdf2djvu
	$(MAKE) -C tests/

MAN_PAGES = $(wildcard doc/*.1 doc/po/*.1)
MO_FILES = $(wildcard po/*.mo)

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) pdf2djvu $(DESTDIR)$(bindir)
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

# vim:ts=4 sw=4 noet
