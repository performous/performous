# This file is part of mingw-cross-env.
# See doc/index.html for further information.

# libschroedinger
PKG             := libschroedinger
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 1.0.9
$(PKG)_CHECKSUM := b4121e10cc474c97676e03ae49c5a91c11956ba0
$(PKG)_SUBDIR   := schroedinger-$($(PKG)_VERSION)
$(PKG)_FILE     := schroedinger-$($(PKG)_VERSION).tar.gz
$(PKG)_WEBSITE  := http://diracvideo.org/
$(PKG)_URL      := http://diracvideo.org/download/schroedinger/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc liborc

define $(PKG)_UPDATE
    wget -q -O- 'http://FIXME' | \
    grep '<a href=' | \
    $(SED) -n "s,.*<a href='[^']*/tag/?id=v\\([0-9][^']*\\)'.*,\\1,p" | \
    head -1
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
        --host='$(TARGET)' \
        --disable-shared \
        --prefix='$(PREFIX)/$(TARGET)'
    $(MAKE) -C '$(1)' -j '$(JOBS)'
    $(MAKE) -C '$(1)' -j 1 install
endef
