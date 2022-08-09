# Copyright © 2009-2022 Jakub Wilk <jwilk@jwilk.net>
#
# This file is part of pdf2djvu.
#
# pdf2djvu is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# pdf2djvu is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

usr = $(CURDIR)
host = i686-w64-mingw32
docbook_xsl = http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

rst2html = $(notdir $(shell command -v rst2html || echo rst2html.py)) \
	--input-encoding=UTF-8 \
	--strict

ifeq "$(wildcard ../.git)" ""
version := $(shell IFS='()' read _ v _ < ../doc/changelog && echo $$v)
orig_src_tarball = pdf2djvu-$(version).tar.xz
else
orig_src_tarball := $(shell ../private/build-source-tarball --print-name)
version = $(word 2,$(subst .tar, ,$(subst -, ,$(orig_src_tarball))))
endif

ifeq "$(orig_src_tarball)" ""
$(error cannot determine orig source tarball name)
endif

djvulibre_tools = bzz c44 cjb2 csepdjvu djvuextract djvumake djvused djvmcvt

download =
untar = tar --strip-components=1 -xf
ship_dll = install -m 644 bin/$1*.dll dist/ && $(STRIP) dist/$1*.dll

cmake_options = \
	-DCMAKE_SYSTEM_NAME=Windows \
	-DCMAKE_INSTALL_PREFIX=$(usr) \
	-DCMAKE_FIND_ROOT_PATH=$(usr) \
	-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
	-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \

export CC = $(host)-gcc
export CXX = $(host)-g++
export AS = $(host)-as
export DLLTOOL = $(host)-dlltool
export OBJDUMP = $(host)-objdump
export STRIP = $(host)-strip

export PKG_CONFIG_PATH = $(usr)/lib/pkgconfig
export CPATH = $(usr)/include
export LDFLAGS = -L$(usr)/lib

export CFLAGS = -Os
export CXXFLAGS = -Os -std=gnu++17

export V = 1
export VERBOSE = 1

export WINEDEBUG ?= err+all,fixme-all
export WINE = wine
export WINESERVER = /usr/lib/wine/wineserver32

ifneq "$(download)" ""
export PDF2DJVU_DOWNLOAD = 1
endif

.PHONY: all
all:
	@echo Usage:
	@echo '  make source-dist'
	@echo '  make binary-dist'

.PHONY: clean

# source tarballs

src_tarball_topdir = pdf2djvu-win32-source-$(version)
src_tarball_name = pdf2djvu-win32-source-$(version)
tar_opts = --owner root --group root --mode u=rwX,go=rX --format ustar --sort name
xz_opts = -8e

.PHONY: source-dist
source-dist:
	$(MAKE) $(shell grep '/source[-]stamp:' Makefile | cut -d: -f1)
	rm -rf $(src_tarball_topdir)
	mkdir $(src_tarball_topdir)
	cp -rl src/*/ $(src_tarball_topdir)/
	find $(src_tarball_topdir) -name source-stamp -delete
	find $(src_tarball_topdir) -type d -name .pc | xargs rm -rf
	tar $(tar_opts) -I 'xz -v $(xz_opts)' -cf $(src_tarball_name).tar.xz $(src_tarball_topdir)
	rm -rf $(src_tarball_topdir)

.PHONY: clean-source
clean: clean-source
clean-source:
	rm -rf src/
	rm -rf $(src_tarball_topdir)
	rm -f $(src_tarball_name).tar.xz

# binary zipballs

zipball_topdir = pdf2djvu-$(version)
zipball_name = pdf2djvu-win32-$(version)
zip_opts = -9 -X -r

.PHONY: binary-dist
binary-dist: install
	rm -rf $(zipball_topdir)
	mkdir $(zipball_topdir)
	! strings -f dist/*.exe dist/*.dll | grep -F '$(CURDIR)'
	! strings -l e -f dist/*.exe dist/*.dll | grep -F '$(CURDIR)'
	cp -rl dist/* $(zipball_topdir)
	chmod a-x $(zipball_topdir)/*.exe
	chmod -R u=rwX,go=rX $(zipball_topdir)
	zip $(zip_opts) $(zipball_name).zip $(zipball_topdir)
	rm -r $(zipball_topdir)

.PHONY: clean-binary
clean: clean-binary
clean-binary:
	rm -rf bin/ etc/ include/ lib/ share/
	rm -rf dist/
	rm -rf $(zipball_topdir)
	rm -f $(zipball_name).zip

.PHONY: install
install: src/pdf2djvu/install-stamp
install: src/libs-install-stamp
install: src/urw-fonts/install-stamp
install: src/poppler-data/install-stamp

# pdf2djvu

src/pdf2djvu/source-stamp: ../dist/$(orig_src_tarball)
	mkdir -p $(dir $@)
	$(untar) $(<) -C $(dir $@)
	rm -f $(dir $@)/paths.hh
	touch $(@)

src/pdf2djvu/config.status: src/pdf2djvu/source-stamp
src/pdf2djvu/config.status: src/djvulibre/install-stamp
src/pdf2djvu/config.status: src/gettext/install-stamp
src/pdf2djvu/config.status: src/graphicsmagick/install-stamp
src/pdf2djvu/config.status: src/exiv2/install-stamp
src/pdf2djvu/config.status: src/mingwrt/install-stamp
src/pdf2djvu/config.status: src/poppler/install-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--bindir=$(usr)/dist/ \
		--localedir=$(usr)/dist/locale/ \
		--with-graphicsmagick \
		--enable-xmp \
		--disable-openmp

src/pdf2djvu/build-stamp: src/pdf2djvu/config.status
	$(MAKE) djvulibre_bindir=. localedir=./locale -C $(dir $@)
	touch $(@)

src/pdf2djvu/install-stamp: src/pdf2djvu/build-stamp
	install -d dist/doc/
	$(MAKE) install -C $(dir $@)
	$(STRIP) dist/pdf2djvu.exe
	xsltproc --nonet $(docbook_xsl) src/pdf2djvu/doc/manpage.xml > dist/doc/manpage.html
	$(MAKE) -C $(dir $@)/doc/po/ xml
	cd src/pdf2djvu/doc/po/ && set -e && \
		for file in *.xml; do \
			locale="$${file%.*}"; \
			xsltproc --nonet $(docbook_xsl) $$file > $(usr)/dist/doc/manpage-$$locale.html; \
		done
	sed -e 's/$$/\r/' < src/pdf2djvu/doc/COPYING > dist/doc/COPYING.txt
	cd src/pdf2djvu/doc && \
		ls README credits djvudigital \
		| xargs -I{file} -t $(rst2html) {file} $(usr)/dist/doc/{file}.html
	sed -e 's/$$/\r/' < src/pdf2djvu/doc/changelog > dist/doc/changelog.txt
	touch $(@)

# various libraries

src/libs-install-stamp: src/djvulibre/install-stamp
src/libs-install-stamp: src/fontconfig/install-stamp
src/libs-install-stamp: src/freetype/install-stamp
src/libs-install-stamp: src/gettext/install-stamp
src/libs-install-stamp: src/graphicsmagick/install-stamp
src/libs-install-stamp: src/libjpeg/install-stamp
src/libs-install-stamp: src/expat/install-stamp
src/libs-install-stamp: src/exiv2/install-stamp
src/libs-install-stamp: src/poppler/install-stamp
src/libs-install-stamp: src/winiconv/install-stamp
	install -d dist/doc/
	$(rst2html) doc/COPYING.libs.txt > dist/doc/COPYING.libs.html
	touch $(@)

# Poppler

src/poppler/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://poppler.freedesktop.org/poppler-22.08.0.tar.xz b493328721402f25cb7523f9cdc2f7d7c59f45ad999bde75c63c90604db0f20b
	$(untar) src.tar/poppler-22.08.0.tar.xz -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/poppler quilt push -a
	rm -rf $(dir $@)/*.cache $(dir $@)/*~
	touch $(@)

src/poppler/Makefile: src/poppler/source-stamp
src/poppler/Makefile: src/fontconfig/install-stamp
src/poppler/Makefile: src/libjpeg/install-stamp
src/poppler/Makefile: src/openjpeg/install-stamp
	cd $(dir $@) && \
		cmake $(cmake_options) . \
		-DBUILD_CPP_TESTS=OFF \
		-DBUILD_GTK_TESTS=OFF \
		-DBUILD_MANUAL_TESTS=OFF \
		-DBUILD_QT5_TESTS=OFF \
		-DENABLE_BOOST=OFF \
		-DENABLE_CMS=none \
		-DENABLE_CPP=OFF \
		-DENABLE_GLIB=OFF \
		-DENABLE_GTK_DOC=OFF \
		-DENABLE_LIBCURL=OFF \
		-DENABLE_LIBOPENJPEG=openjpeg2 \
		-DENABLE_QT5=OFF \
		-DENABLE_QT6=OFF \
		-DENABLE_SPLASH=ON \
		-DENABLE_UNSTABLE_API_ABI_HEADERS=ON \
		-DENABLE_UTILS=OFF \
		-DFONT_CONFIGURATION=fontconfig \
		-DTESTDATADIR=/nonexistent \
		-DWITH_NSS3=OFF \
		-DWITH_PNG=OFF \
		-DWITH_TIFF=OFF
	find $(dir $@) -name build.make -print0 \
	| xargs -0 sed -i -e 's,\([ 	]\)\(/[^ 	]\+/\($(CC)\|$(CXX)\)\) ,\1$(CURDIR)/tools/enforce-relative-paths \2 ,'

include/dummy-mutex: misc/dummy-mutex.hh
	install -m 644 $(<) $(@)

src/poppler/build-stamp: src/poppler/Makefile include/dummy-mutex
	$(MAKE) -C $(dir $@)
	touch $(@)

src/poppler/install-stamp: src/poppler/build-stamp
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libpoppler)
	touch $(@)

# poppler-data

src/poppler-data/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://poppler.freedesktop.org/poppler-data-0.4.11.tar.gz 2cec05cd1bb03af98a8b06a1e22f6e6e1a65b1e2f3816cb3069bb0874825f08c
	$(untar) src.tar/poppler-data-0.4.11.tar.gz -C $(dir $@)
	touch $(@)

src/poppler-data/build-stamp: src/poppler-data/source-stamp
	$(MAKE) -C $(dir $@)
	touch $(@)

src/poppler-data/install-stamp: src/poppler-data/build-stamp
	$(MAKE) -C $(dir $@) install prefix=$(usr)
	install -d dist/share/
	cp -a share/poppler dist/share/
	install -d dist/doc/
	$(rst2html) doc/COPYING.poppler-data.txt > dist/doc/COPYING.poppler-data.html
	touch $(@)

# DjVuLibre

src/djvulibre/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://downloads.sourceforge.net/project/djvu/DjVuLibre/3.5.28/djvulibre-3.5.28.tar.gz fcd009ea7654fde5a83600eb80757bd3a76998e47d13c66b54c8db849f8f2edc
	$(untar) src.tar/djvulibre-3.5.28.tar.gz -C $(dir $@)
	sed -i -e 's/[A-Z]\+_EXPORT/DLL_EXPORT/' $(dir $@)/libdjvu/*.h
	sed -i -e 's/#if.*def.*AUTOCONF.*/#if 0/' $(dir $@)/libdjvu/DjVuMessage.cpp
	touch $(@)

src/djvulibre/config.status: src/djvulibre/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--without-jpeg \
		--disable-static

src/djvulibre/build-stamp: src/djvulibre/config.status
	$(MAKE) -C $(dir $@)/libdjvu
	$(MAKE) -C $(dir $@)/tools
	touch $(@)

src/djvulibre/install-stamp: src/djvulibre/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@)/libdjvu install
	$(MAKE) -C $(dir $@)/tools install
	set -e && \
	for tool in $(djvulibre_tools); do \
		install bin/$$tool.exe bin/$$tool; \
		install bin/$$tool.exe dist/; \
		$(STRIP) dist/$$tool.exe; \
	done
	$(call ship_dll,libdjvulibre)
	touch $(@)

# GraphicsMagick

src/graphicsmagick/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://downloads.sourceforge.net/project/graphicsmagick/graphicsmagick/1.3.38/GraphicsMagick-1.3.38.tar.xz d60cd9db59351d2b9cb19beb443170acaa28f073d13d258f67b3627635e32675
	$(untar) src.tar/GraphicsMagick-1.3.38.tar.xz -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/graphicsmagick quilt push -a
	cd $(dir $@) && autoreconf -vfi
	rm -rf $(dir $@)/*.cache $(dir $@)/magick/*~
	touch $(@)

src/graphicsmagick/config.status: src/graphicsmagick/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
			--disable-installed \
			--disable-openmp --without-threads \
			--enable-shared --disable-static \
			--with-quantum-depth=8 \
			--without-frozenpaths \
			--without-perl \
			--without-bzlib \
			--without-dps \
			--without-fpx \
			--without-jbig \
			--without-webp \
			--without-jpeg \
			--without-jp2 \
			--without-lcms2 \
			--without-lzma \
			--without-png \
			--without-tiff \
			--without-trio \
			--without-ttf \
			--without-umem \
			--without-wmf \
			--without-xml \
			--without-zlib \
			--without-x \
			--without-png \
			--without-gdi32

src/graphicsmagick/build-stamp: src/graphicsmagick/config.status
	$(MAKE) -C $(dir $@)
	touch $(@)

src/graphicsmagick/install-stamp: src/graphicsmagick/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libGraphicsMagick)
	rm dist/libGraphicsMagickWand*.dll
	install -d dist/share/GraphicsMagick
	install -m 644 share/GraphicsMagick-1.3.38/config/*.mgk dist/share/GraphicsMagick
	touch $(@)

# Expat

src/expat/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://github.com/libexpat/libexpat/releases/download/R_2_4_8/expat-2.4.8.tar.xz f79b8f904b749e3e0d20afeadecf8249c55b2e32d4ebb089ae378df479dcaf25
	$(untar) src.tar/expat-2.4.8.tar.xz -C $(dir $@)
	touch $(@)

src/expat/config.status: src/expat/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--without-docbook \
		--disable-static

src/expat/build-stamp: src/expat/config.status
	$(MAKE) -C $(dir $@)
	touch $(@)

src/expat/install-stamp: src/expat/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libexpat)
	touch $(@)

# Exiv2

src/exiv2/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://github.com/Exiv2/exiv2/releases/download/v0.27.5/exiv2-0.27.5-Source.tar.gz -n exiv2-0.27.5.tar.gz 35a58618ab236a901ca4928b0ad8b31007ebdc0386d904409d825024e45ea6e2
	$(untar) src.tar/exiv2-0.27.5.tar.gz -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/exiv2 quilt push -a
	touch $(@)

src/exiv2/Makefile: src/exiv2/source-stamp
src/exiv2/Makefile: src/expat/install-stamp
	cd $(dir $@) && cmake $(cmake_options) . \
		-DEXIV2_BUILD_EXIV2_COMMAND=OFF \
		-DEXIV2_BUILD_SAMPLES=OFF \
		-DEXIV2_ENABLE_PNG=OFF \
		-DEXIV2_ENABLE_PRINTUCS2=OFF \
		-DEXIV2_ENABLE_LENSDATA=OFF \
		-DEXIV2_ENABLE_DYNAMIC_RUNTIME=OFF
	find $(dir $@) -name build.make -print0 \
	| xargs -0 sed -i -e 's, \(/[^ ]\+/\($(CC)\|$(CXX)\)\) , $(CURDIR)/tools/enforce-relative-paths \1 ,'

src/exiv2/build-stamp: src/exiv2/Makefile
	$(MAKE) -C $(dir $@)
	touch $(@)

src/exiv2/install-stamp: src/exiv2/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libexiv2)
	touch $(@)

# FreeType

src/freetype/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://download.savannah.gnu.org/releases/freetype/freetype-2.12.1.tar.xz 4766f20157cc4cf0cd292f80bf917f92d1c439b243ac3018debf6b9140c41a7f
	$(untar) src.tar/freetype-2.12.1.tar.xz -C $(dir $@)
	cd $(dir $@) && ./autogen.sh
	rm -rf $(dir $@)/builds/unix/*.cache/
	touch $(@)

src/freetype/config.status: src/freetype/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--without-png \
		--without-zlib \
		--without-brotli \
		--without-harfbuzz \
		--disable-static
	touch $(@)

src/freetype/build-stamp: src/freetype/config.status
	$(MAKE) -C $(dir $@)
	touch $(@)

src/freetype/install-stamp: src/freetype/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libfreetype)
	touch $(@)

# fontconfig

src/fontconfig/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.1.tar.bz2 f655dd2a986d7aa97e052261b36aa67b0a64989496361eca8d604e6414006741
	$(untar) src.tar/fontconfig-2.13.1.tar.bz2 -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/fontconfig quilt push -a
	cd $(dir $@) && autoreconf -vfi
	rm -rf $(dir $@)/*.cache $(dir $@)/*~
	touch $(@)

src/fontconfig/config.status: src/fontconfig/source-stamp
src/fontconfig/config.status: src/freetype/install-stamp
src/fontconfig/config.status: src/expat/install-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--with-add-fonts=CUSTOMFONTDIR \
		--disable-static \
		--disable-docs

src/fontconfig/build-stamp: src/fontconfig/config.status
	$(MAKE) -C $(dir $@)/
	touch $(@)

src/fontconfig/install-stamp: src/fontconfig/build-stamp
	install -d dist/etc/fonts/conf.d/
	$(MAKE) -C $(dir $@)/ install RUN_FC_CACHE_TEST=false
	$(call ship_dll,libfontconfig)
	install -m 644 etc/fonts/fonts.conf dist/etc/fonts/
	install -m 644 etc/fonts/conf.d/[3-6]*.conf dist/etc/fonts/conf.d/
	install -m 644 misc/[3-6]*.conf dist/etc/fonts/conf.d/
	touch $(@)

# GNU gettext

src/gettext/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://ftp.gnu.org/pub/gnu/gettext/gettext-0.19.8.1.tar.xz 105556dbc5c3fbbc2aa0edb46d22d055748b6f5c7cd7a8d99f8e7eb84e938be4
	$(untar) src.tar/gettext-0.19.8.1.tar.xz --exclude='gettext-tools' -C $(dir $@)
	sed -E -i -e '/(#undef|#define|extern .*) (v?sn?printf|[*]?setlocale)/d' $(dir $@)/gettext-runtime/intl/libgnuintl.in.h
	sed -E -i -e 's/-D(ENABLE_RELOCATABLE)=1/-U\1/' $(dir $@)/gettext-runtime/intl/Makefile.in
	touch $(@)

src/gettext/config.status: src/gettext/source-stamp
src/gettext/config.status: src/winiconv/install-stamp
	cp bin/iconv.dll $(dir $@)/gettext-runtime/
	cd $(dir $@)/gettext-runtime/ && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static \
		--disable-libasprintf \
		--disable-threads
	touch $(@)

src/gettext/build-stamp: src/gettext/config.status
	$(MAKE) -C $(dir $@)/gettext-runtime/intl/ \
		aliaspath='\""; return NULL; "\"' \
		localedir='.'
	touch $(@)

src/gettext/install-stamp: src/gettext/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@)/gettext-runtime/intl/ install
	$(call ship_dll,libintl)
	touch $(@)

# winiconv

src/winiconv/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://github.com/win-iconv/win-iconv/archive/v0.0.8.tar.gz -n win-iconv-0.0.8.tar.gz 23adea990a8303c6e69e32a64a30171efcb1b73824a1c2da1bbf576b0ae7c520
	$(untar) src.tar/win-iconv-0.0.8.tar.gz -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/winiconv quilt push -a
	touch $(@)

src/winiconv/build-stamp: src/winiconv/source-stamp
	$(MAKE) -C $(dir $@)
	touch $(@)

src/winiconv/install-stamp: src/winiconv/build-stamp
	$(MAKE) -C $(dir $@) install prefix=$(usr)
	$(call ship_dll,iconv)
	touch $(@)

# libjpeg

src/libjpeg/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://jpegclub.org/support/files/jpegsrc.v6b2.tar.gz 6d2be0db82dda2e50c908b72aa22ed8cf42cc5aa5ae5f2b081498be6ef16eabc
	$(untar) src.tar/jpegsrc.v6b2.tar.gz -C $(dir $@)
	cd $(dir $@) && QUILT_PATCHES=../../patches/libjpeg quilt push -a
	cd $(dir $@) && ACLOCAL=aclocal-1.11 AUTOMAKE=automake-1.11 autoreconf -vfi
	rm -rf $(dir $@)/*.cache $(dir $@)/*~
	touch $(@)

src/libjpeg/config.status: src/libjpeg/source-stamp
	cd $(dir $@) && \
		./configure --host=$(host) --prefix=$(usr) \
		--disable-static

src/libjpeg/build-stamp: src/libjpeg/config.status
	$(MAKE) -C $(dir $@)
	touch $(@)

src/libjpeg/install-stamp: src/libjpeg/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libjpeg)
	touch $(@)

# OpenJPEG

src/openjpeg/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://github.com/uclouvain/openjpeg/archive/v2.5.0.tar.gz -n openjpeg-2.5.0.tar.gz 0333806d6adecc6f7a91243b2b839ff4d2053823634d4f6ed7a59bc87409122a
	$(untar) src.tar/openjpeg-2.5.0.tar.gz -C $(dir $@)
	rm -rf $(dir $@)/thirdparty/*/
	touch $(@)

src/openjpeg/Makefile: src/openjpeg/source-stamp
	mkdir -p $(dir $@)
	cd $(dir $@) && cmake $(cmake_options) . \
		-DBUILD_THIRDPARTY=OFF
	find $(dir $@) -name build.make -print0 \
	| xargs -0 sed -i -e 's, \(/[^ ]\+/\($(CC)\|$(CXX)\)\) , $(CURDIR)/tools/enforce-relative-paths \1 ,'

src/openjpeg/build-stamp: src/openjpeg/Makefile
	$(MAKE) -C $(dir $@)
	touch $(@)

src/openjpeg/install-stamp: src/openjpeg/build-stamp
	install -d dist/
	$(MAKE) -C $(dir $@) install
	$(call ship_dll,libopenjp2)
	touch $(@)

# URW fonts

standard-fonts = # replacements for 14 standard Type 1 fonts:
standard-fonts += d050000l.pfb # ZapfDingbats -> Dingbats
standard-fonts += n019003l.pfb # Helvetica -> Nimbus Sans L Regular
standard-fonts += n019004l.pfb # Helvetica−Bold -> Nimbus Sans L Bold
standard-fonts += n019023l.pfb # Helvetica−Oblique -> Nimbus Sans L Regular Italic
standard-fonts += n019024l.pfb # Helvetica−BoldOblique -> Nimbus Sans L Bold Italic
standard-fonts += n021003l.pfb # Times−Roman -> Nimbus Roman No9 L Regular
standard-fonts += n021004l.pfb # Times−Bold -> Nimbus Roman No9 L Medium
standard-fonts += n021023l.pfb # Times−Italic -> Nimbus Roman No9 L Regular Italic
standard-fonts += n021024l.pfb # Times−BoldItalic -> Nimbus Roman No9 L Medium Italic
standard-fonts += n022003l.pfb # Courier -> Nimbus Mono L Regular
standard-fonts += n022004l.pfb # Courier−Bold -> Nimbus Mono L Bold
standard-fonts += n022023l.pfb # Courier−Oblique -> Nimbus Mono L Regular Oblique
standard-fonts += n022024l.pfb # Courier−BoldOblique -> Nimbus Mono L Bold Oblique
standard-fonts += s050000l.pfb # Symbol -> Standard Symbols L

src/urw-fonts/source-stamp:
	mkdir -p $(dir $@)
	tools/wget https://src.fedoraproject.org/repo/pkgs/urw-fonts/urw-fonts-1.0.7pre44.tar.bz2/51c6c2690593cd9bd92f197a6f2ff8bd/urw-fonts-1.0.7pre44.tar.bz2 df0ad8413f1e4bc0b6e5f964192879b0dc66d7be521f9666357ac30bad4c7a32
	$(untar) src.tar/urw-fonts-1.0.7pre44.tar.bz2 -C $(dir $@) --strip-components=0
	touch $(@)

src/urw-fonts/build-stamp: src/urw-fonts/source-stamp
	touch $(@)

src/urw-fonts/install-stamp: src/urw-fonts/build-stamp
	install -d dist/fonts/ dist/doc/
	install -m 644 $(addprefix $(dir $@)/,$(standard-fonts)) dist/fonts/
	$(rst2html) doc/COPYING.fonts.txt > dist/doc/COPYING.fonts.html
	touch $(@)

# MinGW runtime

src/mingwrt/install-stamp:
	mkdir -p $(dir $@) bin/ dist/
	cp $(shell $(CXX) -print-file-name=libstdc++-6.dll) bin/
	cp $(or $(wildcard $(shell $(CXX) -print-file-name=libgcc_s_dw2-1.dll) $(shell $(CXX) -print-file-name=libgcc_s_sjlj-1.dll))) bin/
	$(call ship_dll,libstdc++)
	$(call ship_dll,libgcc_s)
	touch $(@)

# tests

.PHONY: test
test: install
	env -u FONTCONFIG_FILE -u FONTCONFIG_PATH XDG_CONFIG_HOME=/nonexistent \
	$(MAKE) -C ../tests/ pdf2djvu='$(WINE) ../win32/dist/pdf2djvu.exe' pdf2djvu_win32=1

# vim:ts=4 sts=4 sw=4 noet
