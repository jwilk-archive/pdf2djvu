# Copyright © 2009, 2010, 2011 Jakub Wilk
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 dated June, 1991.

usr = $(CURDIR)
host = i686-w64-mingw32
docbook_xsl = http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

version := $(shell cat ../doc/changelog | head -n1 | cut -d' ' -f2 | tr -d '()')

djvulibre_tools = djvmcvt djvused bzz csepdjvu djvuextract cjb2 c44 djvumake

untar = tar --strip-components=1 -xf
ship_dll = cp bin/$1*.dll dist/ && $(STRIP) dist/$1*.dll && chmod a-x dist/$1*.dll

export CC = $(host)-gcc -D__USE_MINGW_ANSI_STDIO
export CXX = $(host)-g++ -D__USE_MINGW_ANSI_STDIO
export AS = $(host)-as
export DLLTOOL = $(host)-dlltool
export OBJDUMP = $(host)-objdump
export STRIP = $(host)-strip

export PKG_CONFIG_PATH = $(usr)/lib/pkgconfig
export CPATH = $(usr)/include
export LDFLAGS = -L$(usr)/lib

export CFLAGS = -Os
export CXXFLAGS = -Os

.PHONY: all
all:
	@echo Usage:
	@echo '  make source-dist'
	@echo '  make binary-dist'

# Source tarballs

src_tarball_topdir = pdf2djvu-win32-source-$(version)
src_tarball_name = pdf2djvu-win32-source_$(version)
export TAR_OPTIONS = --owner root --group root --mode a+rX

.PHONY:
source-dist:
	rm -rf src/*/
	$(MAKE) $(shell grep '[^ ]*source[-]stamp:' Makefile | cut -d: -f1)
	mkdir -p $(src_tarball_topdir)
	cp -rl src/*/ $(src_tarball_topdir)/
	find $(src_tarball_topdir) -name source-stamp -delete
	tar -cf $(src_tarball_name).tar $(src_tarball_topdir)
	xz -v $(src_tarball_name).tar
	rm -rf $(src_tarball_topdir)

# Binary zipballs

zipball_topdir = pdf2djvu-$(version)
zipball_name = pdf2djvu-win32_$(version)
export ZIPOPT = -9 -X -r

.PHONY: binary-dist
binary-dist: install
	mkdir -p $(zipball_topdir)
	cp -rl dist/* $(zipball_topdir)
	zip $(zipball_name).zip $(zipball_topdir)
	rm -r $(zipball_topdir)

.PHONY: install
install: src/pdf2djvu/install-stamp
install: src/libs-install-stamp
install: src/urw-fonts/install-stamp

# pdf2djvu

src/pdf2djvu/source-stamp: src/pdf2djvu_$(version).tar.gz
	mkdir -p $(dir $@)
	cd $(dir $@) && $(untar) $(CURDIR)/$(<)
	sed -i \
		-e '/^extern/d; /^"C"$$/d; /^const char [*]_nl_/d; s/ + _nl_[a-z_]* + .*$$//' \
		$(dir $@)/configure
	sed -i -e 's/%zu/%Iu/g; s/%ju/%I64u/g' \
		$(dir $@)/*.cc \
		$(dir $@)/*.hh \
		$(dir $@)/po/*.po*
	rm -f $(dir $@)/paths.hh
	touch $@

src/pdf2djvu/config.status: src/pdf2djvu/source-stamp
src/pdf2djvu/config.status: src/djvulibre/install-stamp
src/pdf2djvu/config.status: src/libxslt/install-stamp
src/pdf2djvu/config.status: src/poppler/install-stamp
src/pdf2djvu/config.status: src/gettext/install-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--bindir=$(usr)/dist/ \
		--localedir=$(usr)/dist/locale/ \
		--without-graphicsmagick

src/pdf2djvu/build-stamp: src/pdf2djvu/config.status
	$(MAKE) djvulibre_bindir=. localedir=./locale -C $(dir $@)
	touch $@

src/pdf2djvu/install-stamp: src/pdf2djvu/build-stamp
	mkdir -p dist/doc/
	$(MAKE) install -C $(dir $@)
	mv dist/pdf2djvu dist/pdf2djvu.exe
	$(STRIP) dist/pdf2djvu.exe
	chmod a-x dist/pdf2djvu.exe
	xsltproc --nonet $(docbook_xsl) src/pdf2djvu/doc/pdf2djvu.xml > dist/doc/manual-page.html
	$(MAKE) -C $(dir $@)/doc/po/ xml
	cd src/pdf2djvu/doc/po/ && set -e && \
		for file in pdf2djvu.*.xml; do \
			suffix="$${file#*.}"; \
			locale="$${suffix%.*}"; \
			xsltproc --nonet $(docbook_xsl) $$file > $(usr)/dist/doc/manual-page-$$locale.html; \
		done
	sed -e 's/$$/\r/' < src/pdf2djvu/COPYING > dist/doc/COPYING.txt
	cd src/pdf2djvu/doc && set -e && \
		for file in *.txt; do \
			rst2html $$file > $(usr)/dist/doc/$${file%.txt}.html; \
		done
	sed -e 's/$$/\r/' < src/pdf2djvu/doc/changelog > dist/doc/changelog.txt
	touch $@

# Various libraries

src/libs-install-stamp: src/djvulibre/install-stamp
src/libs-install-stamp: src/fontconfig/install-stamp
src/libs-install-stamp: src/freetype/install-stamp
src/libs-install-stamp: src/gettext/install-stamp
src/libs-install-stamp: src/libjpeg/install-stamp
src/libs-install-stamp: src/libxml2/install-stamp
src/libs-install-stamp: src/libxslt/install-stamp
src/libs-install-stamp: src/poppler/install-stamp
src/libs-install-stamp: src/winiconv/install-stamp
	mkdir -p dist/doc/
	rst2html COPYING.libs.txt > dist/doc/COPYING.libs.html
	touch $@

# Poppler

src/poppler/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://poppler.freedesktop.org/poppler-0.16.7.tar.gz
	echo 'fab4e8af2d34d4142a718819fd747d40930b71ef520ee0e6bcc94e2c44f22bcd  src/poppler-0.16.7.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../poppler-*.tar.gz
	rm src/poppler-*.tar.gz
	touch $@

src/poppler/config.status: src/poppler/source-stamp
src/poppler/config.status: src/fontconfig/install-stamp
src/poppler/config.status: src/libjpeg/install-stamp
	cd $(dir $@) && \
		CPPFLAGS='-DWIN32_LEAN_AND_MEAN -DXMD_H' \
		./configure --host=$(host) --prefix=$(usr) \
		--with-font-configuration=fontconfig \
		--enable-xpdf-headers \
		--disable-static \
		--disable-cms \
		--disable-abiword-output \
		--disable-cairo-output \
		--disable-poppler-cpp \
		--disable-poppler-glib \
		--disable-poppler-qt \
		--disable-poppler-qt4 \
		--disable-utils \
		--disable-libpng \
		--disable-compile-warnings
	sed -i -e 's!^\(#define POPPLER_DATADIR\) .*!\1 "./poppler-data/"!' $(dir $@)/config.h

src/poppler/build-stamp: src/poppler/config.status
	$(MAKE) -C $(dir $@)
	touch $@

src/poppler/install-stamp: src/poppler/build-stamp
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libpoppler)
	touch $@

# DjVuLibre

src/djvulibre/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://downloads.sourceforge.net/djvu/djvulibre-3.5.23.tar.gz
	echo '608c07e629626cfe713168c942a07d2db3b0b19df4aae1f598473f4a2daf213d  src/djvulibre-3.5.23.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../djvulibre-*.tar.gz
	sed -i \
		-e '/LIBS=-lmsvcp60/d' \
		-e 's/mingw32)/mingw32*)/' \
		$(dir $@)/configure
	sed -i -e 's/define USE_WIN32_INTERLOCKED 1/define USE_GCC_I386_ASM 1/' $(dir $@)/libdjvu/atomic.cpp
	sed -i -e 's/#if.*def.*AUTOCONF.*/#if 0/' $(dir $@)/libdjvu/DjVuMessage.cpp
	rm src/djvulibre-*.tar.gz
	touch $@

src/djvulibre/config.status: src/djvulibre/source-stamp
src/djvulibre/config.status: src/libjpeg/config.status
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static

src/djvulibre/build-stamp: src/djvulibre/config.status
	$(MAKE) -C $(dir $@)/libdjvu
	$(MAKE) -C $(dir $@)/tools
	touch $@

src/djvulibre/install-stamp: src/djvulibre/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@)/libdjvu install
	$(MAKE) -C $(dir $@)/tools install
	set -e && \
	for tool in $(djvulibre_tools); do \
		cp bin/$$tool dist/$$tool.exe; \
		$(STRIP) dist/$$tool.exe; \
		chmod a-x dist/$$tool.exe; \
	done
	$(call ship_dll,libdjvulibre)
	touch $@

# libxml2

src/libxml2/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://xmlsoft.org/sources/libxml2-2.7.8.tar.gz
	echo 'cda23bc9ebd26474ca8f3d67e7d1c4a1f1e7106364b690d822e009fdc3c417ec  src/libxml2-2.7.8.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../libxml2-*.tar.gz
	rm src/libxml2-*.tar.gz
	rm -r src/libxml2/test
	rm -r src/libxml2/result
	touch $@

src/libxml2/config.status: src/libxml2/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static \
		--with-minimum \
		--with-html \
		--with-output \
		--with-tree \
		--with-writer \
		--with-xpath \
		--with-sax1

src/libxml2/build-stamp: src/libxml2/config.status
	$(MAKE) -C $(dir $@)
	touch $@

src/libxml2/install-stamp: src/libxml2/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@) install
	cp xmlexports.h include/libxml2/libxml/xmlexports.h
	$(call ship_dll,libxml2)
	touch $@

# libxslt

src/libxslt/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://xmlsoft.org/sources/libxslt-1.1.26.tar.gz
	echo '55dd52b42861f8a02989d701ef716d6280bfa02971e967c285016f99c66e3db1  src/libxslt-1.1.26.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../libxslt-*.tar.gz
	rm src/libxslt-*.tar.gz
	sed -i -e 's/^\(#if defined(WIN32)\).*/\1/' src/libxslt/libxslt/libxslt.h
	touch $@

src/libxslt/config.status: src/libxslt/source-stamp
src/libxslt/config.status: src/libxml2/install-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static \
		--with-libxml-prefix=$(usr) \
		--with-libxml-include-prefix=$(usr)/include/libxml2/ \
		--without-crypto \
		--without-debug \
		--without-debugger \
		--without-plugins

src/libxslt/build-stamp: src/libxslt/config.status
	$(MAKE) -C $(dir $@)/libxslt/
	touch $@

src/libxslt/install-stamp: src/libxslt/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@)/libxslt/ install
	$(call ship_dll,libxslt)
	touch $@

# FreeType

src/freetype/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://download.savannah.gnu.org/releases/freetype/freetype-2.4.4.tar.bz2
	echo '4b8281c7dc4d375c6b65d3c6f4808e488a313fab47d7be82aad2c871c8480fef  src/freetype-2.4.4.tar.bz2' | sha256sum -c
	cd $(dir $@) && $(untar) ../freetype-*.tar.bz2
	rm src/freetype-*.tar.bz2
	touch $@

src/freetype/config.status: src/freetype/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--without-zlib \
		--disable-static
	touch $@

src/freetype/build-stamp: src/freetype/config.status
	$(MAKE) -C $(dir $@)
	touch $@

src/freetype/install-stamp: src/freetype/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libfreetype)
	touch $@

# fontconfig

src/fontconfig/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://www.freedesktop.org/software/fontconfig/release/fontconfig-2.8.0.tar.gz
	echo 'fa2a1c6eea654d9fce7a4b1220f10c99cdec848dccaf1625c01f076b31382335  src/fontconfig-2.8.0.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../fontconfig-*.tar.gz
	rm src/fontconfig-*.tar.gz
	sed -i -e 's/"append"/"prepend"/g' src/fontconfig/conf.d/*-urw-aliases.conf
	touch $@

src/fontconfig/config.status: src/fontconfig/source-stamp
src/fontconfig/config.status: src/freetype/install-stamp
src/fontconfig/config.status: src/libxml2/install-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--with-freetype-config=$(usr)/freetype-config \
		--with-add-fonts=CUSTOMFONTDIR \
		--enable-libxml2 \
		--disable-static \
		--disable-docs

src/fontconfig/build-stamp: src/fontconfig/config.status
	$(MAKE) -C $(dir $@)/
	touch $@

src/fontconfig/install-stamp: src/fontconfig/build-stamp
	mkdir -p dist/etc/fonts/conf.d/
	$(MAKE) -C $(dir $@)/ install RUN_FC_CACHE_TEST=false
	$(call ship_dll,libfontconfig)
	cp etc/fonts/fonts.conf dist/etc/fonts/
	cp etc/fonts/fonts.dtd dist/etc/fonts/
	cp etc/fonts/conf.d/[3-6]*.conf dist/etc/fonts/conf.d/
	touch $@

# GNU gettext

src/gettext/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.1.1.tar.gz
	echo '93ac71a7afa5b70c1b1032516663658c51e653087f42a3fc8044752c026443e9  src/gettext-0.18.1.1.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../gettext-*.tar.gz --exclude='gettext-tools'
	rm src/gettext-*.tar.gz
	sed -r -i -e '/(#undef|#define|extern .*) (v?sn?printf|[*]?setlocale)/d' $(dir $@)/gettext-runtime/intl/libgnuintl.h.in
	sed -r -i -e 's/(-DENABLE_RELOCATABLE)=1/\1=0/' $(dir $@)/gettext-runtime/intl/Makefile.in
	touch $@

src/gettext/config.status: src/gettext/source-stamp
src/gettext/config.status: src/winiconv/install-stamp
	cp lib/libiconv.dll $(dir $@)/gettext-runtime/
	cd $(dir $@)/gettext-runtime/ && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static \
		--disable-libasprintf
	touch $@	

src/gettext/build-stamp: src/gettext/config.status
	$(MAKE) -C $(dir $@)/gettext-runtime/intl/ \
		aliaspath='\""; return NULL; "\"' \
		localedir='.'
	touch $@

src/gettext/install-stamp: src/gettext/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@)/gettext-runtime/intl/ install
	$(call ship_dll,libintl)
	touch $@

# winiconv

src/winiconv/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c ftp://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/win-iconv-dev_tml-20100912_win32.zip
	echo 'a5a8f1f6578ce6fcce88bd0fe3b4e7866d4fc158ee6d5dec164f10b443c82f6e  src/win-iconv-dev_tml-20100912_win32.zip' | sha256sum -c
	unzip src/win-iconv-dev_tml-20100912_win32.zip 'src/tml/win_iconv/*'
	mv src/tml/win_iconv/* $(dir $@)
	rm -r src/tml/
	rm src/win-iconv-*.zip
	sed -i -e '/iconv(/ s/const//' $(dir $@)/iconv.h
	touch $@

src/winiconv/build-stamp: src/winiconv/source-stamp
	$(CC) $(CFLAGS) -shared $(dir $@)/win_iconv.c -o $(dir $@)/libiconv.dll
	touch $@

src/winiconv/install-stamp: src/winiconv/build-stamp
	mkdir -p include/ bin/ lib/ dist/
	cp $(dir $@)/iconv.h include/
	cp $(dir $@)/libiconv.dll bin/
	cp $(dir $@)/libiconv.dll lib/
	$(call ship_dll,libiconv)
	touch $@

# libjpeg

src/libjpeg/source-stamp:
	mkdir -p $(dir $@)
	cd src && wget -c http://ftp.debian.org/pool/main/libj/libjpeg6b/libjpeg6b_6b1.orig.tar.gz
	echo 'dbe673129e69eaaed47e3cfccf86b1582fa19862a3ce8189ea8f8cfc1800d9aa  src/libjpeg6b_6b1.orig.tar.gz' | sha256sum -c
	cd $(dir $@) && $(untar) ../libjpeg6b_*.tar.gz
	rm src/libjpeg6b_*.tar.gz
	touch $@

src/libjpeg/config.status: src/libjpeg/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static

src/libjpeg/build-stamp: src/libjpeg/config.status
	$(MAKE) -C $(dir $@)
	touch $@

src/libjpeg/install-stamp: src/libjpeg/build-stamp
	mkdir -p dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libjpeg)
	touch $@

# URW fonts

src/urw-fonts/source-stamp:
	mkdir -p $(dir $@)
	cd $(dir $@) && wget -A '*.pfb' -r -nd -np http://svn.ghostscript.com/ghostscript/trunk/urw-fonts/
	touch $@

src/urw-fonts/build-stamp: src/urw-fonts/source-stamp
	touch $@

src/urw-fonts/install-stamp: src/urw-fonts/build-stamp
	mkdir -p dist/fonts/ dist/doc/
	cp src/urw-fonts/*.pfb dist/fonts/
	rst2html COPYING.fonts.txt > dist/doc/COPYING.fonts.html
	touch $@

# vim:ts=4 sw=4 noet
