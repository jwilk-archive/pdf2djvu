POPPLER_VERSION = $(shell pkg-config --atleast-version=0.6 poppler && echo 6 || echo 5)
POPPLER_CFLAGS = $(shell pkg-config --cflags poppler) -DPOPPLER_VERSION=$(POPPLER_VERSION)
POPPLER_LDFLAGS = $(shell pkg-config --libs poppler)

LDFLAGS = $(POPPLER_LDFLAGS)
CXXFLAGS = $(POPPLER_CFLAGS) -O3

.PHONY: all
all: pdf2djvu

.PHONY: clean
clean:
	$(RM) pdf2djvu

.PHONY: test
test: pdf2djvu
	./pdf2djvu example.pdf > example.djvu

# vim:ts=4 sw=4 noet
