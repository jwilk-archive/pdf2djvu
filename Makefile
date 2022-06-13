# Copyright © 2007-2022 Jakub Wilk <jwilk@jwilk.net>
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

srcdir = .
include $(srcdir)/autoconf.mk

exe = pdf2djvu$(EXEEXT)

.PHONY: all
all: $(exe)

ifneq "$(WINDRES)" ""
win32-version.o: win32-version.rc autoconf.hh
	$(WINDRES) -c 65001 -o $(@) $(<)
$(exe): win32-version.o
endif

include Makefile.dep

paths.hh: tools/generate-paths-hh
	$(<) $(foreach var,localedir djvulibre_bindir,$(var) $($(var)))

$(exe): config.o
$(exe): debug.o
$(exe): djvu-outline.o
$(exe): i18n.o
$(exe): image-filter.o
$(exe): main.o
$(exe): pdf-backend.o
$(exe): pdf-document-map.o
$(exe): pdf-dpi.o
$(exe): pdf-unicode.o
$(exe): sexpr.o
$(exe): string-format.o
$(exe): string-printf.o
$(exe): string-utils.o
$(exe): sys-command-posix.o
$(exe): sys-command-win32.o
$(exe): sys-encoding.o
$(exe): sys-time.o
$(exe): sys-uuid.o
$(exe): system.o
$(exe): version.o
$(exe): xmp.o
$(exe):
	$(LINK.cc) $(^) $(LDLIBS) -o $(@)

.PHONY: clean
clean:
	rm -f $(exe) *.o paths.hh
	$(MAKE) -C tests/ clean

.PHONY: distclean
distclean: clean
	rm -f autoconf.hh autoconf.mk config.status config.log

.PHONY: vcs-clean
vcs-clean:
	$(MAKE) -C tests/ vcs-clean
	$(MAKE) -C po clean
	$(MAKE) -C doc clean
	$(MAKE) -C doc/po clean
	rm -rf autoconf.hh autoconf.hh.in config.log config.status configure
	rm -rf autom4te.cache
	rm -rf aclocal.m4 m4
	rm -f tools/config.* tools/install-sh
	$(MAKE) distclean

.PHONY: test
test: $(exe)
	$(MAKE) -C tests/

.PHONY: test-installed
test-installed: $(or $(shell command -v pdf2djvu;),$(bindir)/pdf2djvu)
	$(MAKE) -C tests/ pdf2djvu=$(exe)

man_pages = $(wildcard doc/*.1 doc/po/*.1)
mo_files = $(wildcard po/*.mo)

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) $(exe) $(DESTDIR)$(bindir)
	INSTALL='$(INSTALL)' tools/install-manpages $(DESTDIR)$(mandir) $(man_pages)
	INSTALL='$(INSTALL)' tools/install-mo $(DESTDIR)$(localedir) $(mo_files)

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
