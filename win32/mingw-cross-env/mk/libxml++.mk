# This file is part of mingw-cross-env.
# See doc/index.html for further information.

# libxml++
PKG             := libxml++
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 2.30.1
$(PKG)_CHECKSUM := 0fc6fc14c4751754037beb46f387fa7d069fe2c9
$(PKG)_SUBDIR   := libxml++-$($(PKG)_VERSION)
$(PKG)_FILE     := libxml++-$($(PKG)_VERSION).tar.gz
$(PKG)_WEBSITE  := http://libxmlplusplus.sourceforge.net/
$(PKG)_URL      := http://ftp.gnome.org/pub/GNOME/sources/libxml++/$(call SHORT_PKG_VERSION,$(PKG))/$($(PKG)_FILE)
$(PKG)_DEPS     := libxml2 glibmm

define $(PKG)_UPDATE
    wget -q -O- 'http://git.gnome.org/browse/libxml++/refs/tags' | \
    grep '<a href=' | \
    $(SED) -n "s,.*<a href='[^']*/tag/?id=v\\([0-9][^']*\\)'.*,\\1,p" | \
    head -1
endef

define $(PKG)_BUILD
    #$(SED) -i 's,`uname`,MinGW,g' '$(1)/xml2-config.in'
    cd '$(1)' && ./configure \
        --host='$(TARGET)' \
        --disable-shared \
        --without-debug \
        --prefix='$(PREFIX)/$(TARGET)' \
        --without-python \
        --without-threads
    $(MAKE) -C '$(1)' -j '$(JOBS)' bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS=
    $(MAKE) -C '$(1)' -j 1 install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS=
endef
