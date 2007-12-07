PDF_FILES = $(wildcard *.pdf)
DJVU_FILES = $(PDF_FILES:.pdf=.djvu)
SED_FILES = $(DJVU_FILES:.djvu=.djvused)
DUMP_FILES = $(DJVU_FILES:.djvu=.djvudump)

.PHONY: all
all: $(DJVU_FILES) $(SED_FILES) $(DUMP_FILES)

.PHONY: clean
clean:
	$(RM) *.djvu *.djvused *.djvudump

%.djvu: %.pdf ../pdf2djvu
	../pdf2djvu -v $(<) > $(@)

%.djvused: %.djvu
	djvused -e 'output-all; print-outline' $(<) > $(@)
	diff $(@) $(@).orig || true

%.djvudump: %.djvu
	djvudump $(<) > $(@)
	diff $(@) $(@).orig || true

# vim:ts=4 sw=4 noet
