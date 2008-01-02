DJVULIBRE_BIN_PATH = $(shell pkg-config --variable exec_prefix ddjvuapi)/bin
POPPLER_VERSION = $(shell ./tools/get-poppler-version)
EXT_CFLAGS = $(shell pkg-config --cflags poppler-splash ddjvuapi) -DPOPPLER_VERSION=$(POPPLER_VERSION)
EXT_LDFLAGS = $(shell pkg-config --libs poppler-splash ddjvuapi)

CXXFLAGS ?= -Wall -O3
override LDFLAGS += $(EXT_LDFLAGS)
override CXXFLAGS += $(EXT_CFLAGS) -DDJVULIBRE_BIN_PATH="\"$(strip $(DJVULIBRE_BIN_PATH))\""

.PHONY: all
all: pdf2djvu

config.o: config.cc config.h debug.h
debug.o: debug.cc debug.h config.h
pdf2djvu.o: pdf2djvu.cc compoppler.h debug.h config.h
pdf2djvu: pdf2djvu.o debug.o config.o

.PHONY: clean
clean:
	$(RM) pdf2djvu

.PHONY: test
test: pdf2djvu
	$(MAKE) -C tests/

# vim:ts=4 sw=4 noet
