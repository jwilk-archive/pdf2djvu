POPPLER_VERSION = $(shell pkg-config --atleast-version=0.6 poppler && echo 6 || pkg-config --atleast-version=0.5 poppler && echo 5 || echo 4)
EXT_CFLAGS = $(shell pkg-config --cflags poppler ddjvuapi) -DPOPPLER_VERSION=$(POPPLER_VERSION)
EXT_LDFLAGS = $(shell pkg-config --libs poppler ddjvuapi)

CXXFLAGS ?= -O3
override LDFLAGS += $(EXT_LDFLAGS)
override CXXFLAGS += $(EXT_CFLAGS)

.PHONY: all
all: pdf2djvu

.PHONY: clean
clean:
	$(RM) pdf2djvu

.PHONY: test
test: pdf2djvu
	./pdf2djvu example.pdf > example.djvu

# vim:ts=4 sw=4 noet
