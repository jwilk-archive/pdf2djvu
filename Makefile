LDFLAGS = -lpoppler
CXXFLAGS = -I/usr/include/poppler/ -O3

.PHONY: all
all: pdf2djvu

.PHONY: clean
clean:
	$(RM) pdf2djvu

.PHONY: test
test: pdf2djvu
	./pdf2djvu example.pdf > example.djvu

# vim:ts=4 sw=4 noet
