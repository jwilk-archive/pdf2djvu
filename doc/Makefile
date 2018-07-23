srcdir = ..

.PHONY: all
all: pdf2djvu.1

pdf2djvu.1: manpage.xml
	$(srcdir)/tools/build-manpage $(<) $(@)

.PHONY: clean
clean:
	rm -f *.1

.error = GNU make is required

# vim:ts=4 sts=4 sw=4 noet
