#!/bin/sh
MXE_PREFIX=/opt/mxe
PKG_CONFIG=$MXE_PREFIX/usr/bin/i686-pc-mingw32.static-pkg-config

LIBS="atk gobject-2.0 libswscale cairo-fc gthread-2.0 libtiff-4 cairo-ft gtk+-2.0 libxml-2.0 cairo-gobject gtk+-win32-2.0 libxml++-2.6 cairo harfbuzz nettle cairo-pdf hogweed ogg cairo-png icu-i18n opencore-amrnb cairo-ps icu-io opencore-amrwb cairo-script icu-le opencv cairo-svg icu-lx OpenEXR cairo-win32-font icu-uc openssl cairo-win32 IlmBase opus dbus-1 lcms pangocairo eigen3 libavcodec pangoft2 expat libavdevice pango fontconfig libavfilter pangowin32 freetype2 libavformat pixman-1 gail libavutil portaudio-2.0 gdk-2.0 libcroco-0.6 samplerate gdk-pixbuf-2.0 libcrypto gdk-win32-2.0 libffi sigc++-2.0 gio-2.0 libgsf-1 speexdsp giomm-2.4 speex gio-windows-2.0 liblzma sqlite3 glewmx libodbc++ theoradec glew libpcre16 theoraenc glib-2.0 libpcre theora glibmm-2.4 libpcreposix vorbisenc gl libpng16 vorbisfile glu libpng vorbis gmodule-2.0 libpostproc vpx gmodule-export-2.0 librsvg-2.0 x264 gmodule-no-export-2.0 libssl zlib gnutls libswresample"

LINK=`$PKG_CONFIG --libs --static $LIBS`

# No pkgconfig for Boost :(
#LINK="$LINK -L$MXE_PREFIX/usr/i686-pc-mingw32/lib"
#LINK="$LINK -Wl,-Bstatic -lboost_thread_win32-mt -lboost_date_time-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_regex-mt -lboost_system-mt"

echo -n $LINK

