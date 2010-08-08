# This file is part of mingw-cross-env.
# See doc/index.html for further information.

# liborc
PKG             := liborc
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 0.4.5
$(PKG)_CHECKSUM := e24d485093a5e5412b4917e4d89ecd89fad3721b
$(PKG)_SUBDIR   := orc-$($(PKG)_VERSION)
$(PKG)_FILE     := orc-$($(PKG)_VERSION).tar.gz
$(PKG)_WEBSITE  := http://code.entropywave.com/
$(PKG)_URL      := http://code.entropywave.com/download/orc/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc

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
