srcdir = .
include $(srcdir)/Makefile.common

.PHONY: all
all: pdf2djvu

ifneq ($(WINDRES),:)
version.o: version.rc version.hh
	$(WINDRES) -c 1252 -o $(@) $(<)
pdf2djvu: version.o
endif

include Makefile.dep

paths.hh: tools/generate-paths-hh Makefile
	$(<) $(foreach var,localedir djvulibre_bindir,$(var) $($(var)))

pdf2djvu: pdf2djvu.o pdf-backend.o pdf-dpi.o debug.o config.o string-format.o system.o sexpr.o quantizer.o i18n.o
	$(LINK.cc) $(^) $(LDFLAGS) $(LDLIBS) -o $(@) 

.PHONY: clean
clean:
	$(RM) pdf2djvu *.o paths.hh

.PHONY: distclean
distclean: clean
	$(RM) version.hh Makefile config.status config.log

.PHONY: test
test: pdf2djvu
	$(MAKE) -C tests/

MAN_PAGES = $(wildcard doc/*.1)
MO_FILES = $(wildcard po/*.mo)

.PHONY: install
install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) pdf2djvu $(DESTDIR)$(bindir)
ifneq ($(MAN_PAGES),)
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1/
	$(INSTALL) -m 644 $(MAN_PAGES) $(DESTDIR)$(mandir)/man1/
endif
ifneq ($(MO_FILES),)
	for locale in $(basename $(notdir $(MO_FILES))); \
	do \
		$(INSTALL) -d $(DESTDIR)$(localedir)/$$locale/LC_MESSAGES/; \
		$(INSTALL) -m 644 po/$$locale.mo $(DESTDIR)$(localedir)/$$locale/LC_MESSAGES/pdf2djvu.mo; \
	done
endif

# vim:ts=4 sw=4 noet
