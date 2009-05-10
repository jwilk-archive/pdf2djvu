XSL = http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl
XMLLINT = /usr/bin/xmllint --valid --noout --nonet
XSLTPROC = /usr/bin/xsltproc --nonet --param man.charmap.use.subset 0

.PHONY: all
all: pdf2djvu.1

%.1: %.xml
	$(XMLLINT) $(<)
	$(XSLTPROC) $(XSL) $(<)

# vim:ts=4
