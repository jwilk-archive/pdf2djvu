DB2MAN=/usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl
XMLLINT=/usr/bin/xmllint --valid --noout
XSLTPROC=/usr/bin/xsltproc --param man.charmap.use.subset 0

.PHONY: all
all: pdf2djvu.1

%.1: %.xml
	$(XMLLINT) $(<)
	$(XSLTPROC) $(DB2MAN) $(<)

# vim:ts=4
