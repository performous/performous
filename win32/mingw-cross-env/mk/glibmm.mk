# This file is part of mingw-cross-env.
# See doc/index.html for further information.

# GLibmm
PKG             := glibmm
$(PKG)_IGNORE   := 
$(PKG)_VERSION  := 2.24.2
$(PKG)_CHECKSUM := df5f22d2c40ebdf097ecdb4a7dfeef70d1ca24e7
$(PKG)_SUBDIR   := glibmm-$($(PKG)_VERSION)
$(PKG)_FILE     := glibmm-$($(PKG)_VERSION).tar.bz2
$(PKG)_WEBSITE  := http://www.gtkmm.org/
$(PKG)_URL      := http://ftp.gnome.org/pub/GNOME/sources/glibmm/$(call SHORT_PKG_VERSION,$(PKG))/$($(PKG)_FILE)
$(PKG)_DEPS     := glib

define $(PKG)_UPDATE
    wget -q -O- 'http://git.gnome.org/browse/glibmm/refs/tags' | \
    grep '<a href=' | \
    $(SED) -n 's,.*<a[^>]*>glibmm-*\([0-9][^<]*\)<.*,\1,p' | \
    grep -v '^2\.24\.' | \
    head -1
endef

define $(PKG)_BUILD
    # cross build
    # wine confuses the cross-compiling detection, so set it explicitly
    $(SED) -i 's,cross_compiling=no,cross_compiling=yes,' '$(1)/configure'
    cd '$(1)' && ./configure \
        --host='$(TARGET)' \
        --disable-shared \
        --prefix='$(PREFIX)/$(TARGET)' \
        --with-threads=win32 \
        CXX='$(TARGET)-c++' \
        PKG_CONFIG='$(PREFIX)/bin/$(TARGET)-pkg-config'
    $(MAKE) -C '$(1)'    -j '$(JOBS)' install
endef
