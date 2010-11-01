#!/bin/sh -e
# Script to cross-compile Performous's dependency libraries for Win32.
# Copyright (C) 2010 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

die () { echo "$@" >&2 ; exit 1 ; }

assert_binary_on_path () {
  if which "$1" >/dev/null 2>&1; then
    echo found program "$1"
  else
    echo did not find "$1", which is required
    exit 1
  fi
}

if test -z "$CROSS_TOOL_PREFIX"; then
  export CROSS_TOOL_PREFIX=i586-mingw32msvc
fi
echo "Using cross compilers prefixed with '$CROSS_TOOL_PREFIX-'."
echo "(Set CROSS_TOOL_PREFIX to change this; don't include the trailing hyphen.)"
if test -z "$CROSS_GCC"; then
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-gcc
  export CROSS_GCC="$CROSS_TOOL_PREFIX"-gcc
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-g++
  export CROSS_GXX="$CROSS_TOOL_PREFIX"-g++
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-ar
  export CROSS_AR="$CROSS_TOOL_PREFIX"-ar
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-ranlib
  export CROSS_RANLIB="$CROSS_TOOL_PREFIX"-ranlib
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-ld
  export CROSS_LD="$CROSS_TOOL_PREFIX"-ld
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-dlltool
  export CROSS_DLLTOOL="$CROSS_TOOL_PREFIX"-dlltool
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-nm
  export CROSS_NM="$CROSS_TOOL_PREFIX"-nm
  assert_binary_on_path "$CROSS_TOOL_PREFIX"-windres
  export CROSS_WINDRES="$CROSS_TOOL_PREFIX"-windres
fi
if test -z "$WINE"; then
  assert_binary_on_path wine
  export WINE=wine
fi
echo "wine: $WINE"

assert_binary_on_path autoreconf
assert_binary_on_path make
assert_binary_on_path pkg-config
assert_binary_on_path svn
assert_binary_on_path tar
assert_binary_on_path unzip
assert_binary_on_path wget

SCRIPTDIR="`pwd`"
export PREFIX="`pwd`"/deps
export WINEPREFIX="`pwd`"/wine
mkdir -pv "$PREFIX"/bin "$PREFIX"/lib "$PREFIX"/include "$PREFIX"/lib/pkgconfig "$PREFIX"/build-stamps
if test -n "$KEEPTEMP"; then
  RM_RF=true
  echo 'Keeping the built source trees, as you requested.'
else
  RM_RF="rm -rf"
  echo 'Unpacked source trees will be removed after compilation.'
  echo '(Set KEEPTEMP to any value to preserve them.)'
fi

echo 'setting up wine environment'
$WINE reg add 'HKCU\Environment' /v PATH /d Z:"`echo "$PREFIX" | tr '/' '\\'`"\\bin

echo 'creating pkg-config wrapper for cross-compiled environment'
cat >"$PREFIX"/bin/pkg-config <<EOF
#!/bin/sh -e
exec env PKG_CONFIG_LIBDIR='$PREFIX'/lib/pkgconfig '`which pkg-config`' "\$@"
EOF
chmod -v 0755 "$PREFIX"/bin/pkg-config
cat >"$PREFIX"/bin/wine-shwrap <<"EOF"
#!/bin/sh -e
path="`(cd $(dirname "$1") && pwd)`/`basename "$1"`"
echo '#!/bin/bash -e' >"$1"
echo '$WINE '"$path"'.exe "$@" | tr -d '"'\\\015'" >>"$1"
echo 'exit ${PIPESTATUS[0]}' >>"$1"
chmod 0755 "$1"
EOF
chmod 0755 $PREFIX/bin/wine-shwrap

export PATH="$PREFIX"/bin:"$PATH"

download () {
  basename="`basename "$1"`"
  if test ! -f "$basename"; then
    wget -c -O "$basename".part "$1"
    mv -v "$basename".part "$basename"
  fi
}

# Copy the MinGW support DLL if it's in the Debian place.
# (Why said place is what it is is beyond me...)
if test -f /usr/share/doc/mingw32-runtime/mingwm10.dll.gz; then
  gunzip -cf /usr/share/doc/mingw32-runtime/mingwm10.dll.gz >"$PREFIX"/bin/mingwm10.dll
else
  echo "mingwm10.dll is not in the Debian place."
  echo "If it's needed (it's solely a run-time dependency), you'll have to track"
  echo "it down yourself and copy it to $PREFIX/bin."
fi

# We use win-iconv instead of full-fledged GNU libiconv because it still does
# everything the other deps need and is far smaller.
WINICONV="win-iconv-0.0.1"
if test ! -f "$PREFIX"/build-stamps/win-iconv; then
  download http://win-iconv.googlecode.com/files/$WINICONV.tar.bz2
  tar jxvf $WINICONV.tar.bz2
  cd $WINICONV
  make clean
  make -n iconv.dll win_iconv.exe | sed -e 's/^/$CROSS_TOOL_PREFIX-/' | sh -ex
  $CROSS_GCC -mdll -o iconv.dll -Wl,--out-implib,libiconv.a iconv.def win_iconv.o
  cp -v iconv.dll win_iconv.exe "$PREFIX"/bin
  cp -v iconv.h "$PREFIX"/include
  echo '' >>"$PREFIX"/include/iconv.h  # squelch warnings about no newline at the end
  sed -i -e 's://.*$::' "$PREFIX"/include/iconv.h  # squelch warnings about C++ comments
  cp -v libiconv.a "$PREFIX"/lib
  cd ..
  touch "$PREFIX"/build-stamps/win-iconv
  $RM_RF $WINICONV
fi

# zlib
ZLIB="zlib-1.2.5"
if test ! -f "$PREFIX"/build-stamps/zlib; then
  download http://www.zlib.net/$ZLIB.tar.bz2
  tar jxvf $ZLIB.tar.bz2
  cd $ZLIB
  make -f win32/Makefile.gcc PREFIX="$CROSS_TOOL_PREFIX"- zlib1.dll
  cp -v zlib.h zconf.h "$PREFIX"/include
  cp -v zlib1.dll "$PREFIX"/bin
  cp -v libzdll.a "$PREFIX"/lib/libz.a
  cd ..
  touch "$PREFIX"/build-stamps/zlib
  $RM_RF $ZLIB
fi

# libpng
LIBPNG="libpng-1.4.4"
if test ! -f "$PREFIX"/build-stamps/libpng; then
  download http://download.sourceforge.net/libpng/$LIBPNG.tar.gz
  tar zxvf $LIBPNG.tar.gz
  cd $LIBPNG
  make -f scripts/makefile.mingw prefix="$PREFIX" CC="$CROSS_GCC" AR="$CROSS_AR" RANLIB="$CROSS_RANLIB" ZLIBINC="$PREFIX"/include ZLIBLIB="$PREFIX"/lib install-shared
  cd ..
  touch "$PREFIX"/build-stamps/libpng
  $RM_RF $LIBPNG
fi

# Flags passed to every dependency's ./configure script, for those deps that use autoconf and friends.
COMMON_AUTOCONF_FLAGS="--prefix=$PREFIX --host=$CROSS_TOOL_PREFIX --disable-static --enable-shared CPPFLAGS=-I$PREFIX/include LDFLAGS=-L$PREFIX/lib"

# libjpeg
if test ! -f "$PREFIX"/build-stamps/libjpeg; then
  download http://www.ijg.org/files/jpegsrc.v8b.tar.gz
  tar zxvf jpegsrc.v8b.tar.gz
  cd jpeg-8b
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libjpeg
  $RM_RF jpeg-8b
fi

# Runtime (libintl) of GNU Gettext
GETTEXT="gettext-0.18.1.1"
if test ! -f "$PREFIX"/build-stamps/gettext-runtime; then
  download http://ftp.gnu.org/gnu/gettext/$GETTEXT.tar.gz
  tar zxvf $GETTEXT.tar.gz
  cd $GETTEXT/gettext-runtime
  ./configure $COMMON_AUTOCONF_FLAGS --enable-relocatable --disable-libasprintf --disable-java --disable-csharp
  make
  make install
  cd ../..
  touch "$PREFIX"/build-stamps/gettext-runtime
  $RM_RF $GETTEXT
fi

# libxml2
LIBXML2="libxml2-2.7.7"
if test ! -f "$PREFIX"/build-stamps/libxml2; then
  download ftp://xmlsoft.org/libxml2/$LIBXML2.tar.gz
  tar zxvf $LIBXML2.tar.gz
  cd $LIBXML2
  ./configure $COMMON_AUTOCONF_FLAGS --without-python --without-readline
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libxml2
  $RM_RF $LIBXML2
fi

# The bits of Boost that we need.
BOOST="boost_1_43_0"
if test ! -f "$PREFIX"/build-stamps/boost; then
  download http://download.sourceforge.net/boost/$BOOST.tar.bz2
  tar jxvf $BOOST.tar.bz2
  cd $BOOST/tools/jam/src
  ./build.sh
  cp -v bin.linux*/bjam ../../..
  cd ../../..
  cat >>tools/build/v2/user-config.jam <<EOF
using gcc : debian_mingw32_cross : $CROSS_GXX ;
EOF
  ./bjam --prefix="$PREFIX" --with-filesystem --with-system --with-thread --with-date_time --with-program_options --with-regex toolset=gcc-debian_mingw32_cross target-os=windows variant=release link=shared runtime-link=shared threading=multi threadapi=win32 stage
  cp -av boost "$PREFIX"/include
  cp -v stage/lib/*.dll "$PREFIX"/bin
  cp -v stage/lib/*.a "$PREFIX"/lib
  ln -svf libboost_thread_win32.dll.a "$PREFIX"/lib/libboost_thread.dll.a  # so the cmake scripts detect it correctly
  cd ..
  touch "$PREFIX"/build-stamps/boost
  $RM_RF $BOOST
fi

# A package of DirectX headers and implibs kindly made available by the
# SDL folks.  These are used by SDL (to access DirectDraw, DirectSound,
# and DirectInput) and PortAudio (to access DirectSound).
if test ! -f "$PREFIX"/build-stamps/directx; then
  download http://www.libsdl.org/extras/win32/common/directx-source.tar.gz
  tar zxvf directx-source.tar.gz
  cd directx
  sed -i -e "s/dlltool /$CROSS_DLLTOOL /" -e "s/ar /$CROSS_AR /" lib/Makefile
  make -C lib distclean
  make -C lib CC="$CROSS_GCC"
  cp -v include/*.h "$PREFIX"/include
  cp -v lib/*.a "$PREFIX"/lib
  cd ..
  touch "$PREFIX"/build-stamps/directx
  $RM_RF directx
fi

# This is the latest svn revision that works with the SDL-provided
# DirectX header package.
if test ! -f "$PREFIX"/build-stamps/portaudio; then
  if test ! -f portaudio/svn-stamp; then
    svn co -r 1433 http://www.portaudio.com/repos/portaudio/trunk portaudio
    echo 1433 >portaudio/svn-stamp
  else
    if test x"`svn info portaudio | grep '^Revision: '`" != x"Revision: `cat portaudio/svn-stamp`"; then
      svn revert portaudio/configure portaudio/configure.in
      svn up -r "`cat portaudio/svn-stamp`" portaudio
    fi
  fi
  cd portaudio
  echo 'Patching mingw host triplet pattern matching bug in configure.'
  sed -i -e 's/\**mingw\*.*)/\*&/' configure.in
  autoreconf
  ./configure $COMMON_AUTOCONF_FLAGS --with-winapi=directx --with-dxdir="$PREFIX"
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/portaudio
  $RM_RF portaudio
fi

# PortMidi, which unfortunately lacks a decent build system.
# (Yes, I do see the CMakeLists.txt there, but it insists on building
# some things that won't fly in this environment.)
if test ! -f "$PREFIX"/build-stamps/portmidi; then
  download http://download.sourceforge.net/portmedia/portmidi-src-200.zip
  unzip -o portmidi-src-200.zip
  cd portmidi
  $CROSS_GCC -g -O2 -W -Wall -Iporttime -DNDEBUG -D_WINDLL -mdll -o porttime.dll -Wl,--out-implib,libporttime.a porttime/ptwinmm.c -lwinmm
  $CROSS_GCC -g -O2 -W -Wall -Ipm_common -Iporttime -DNDEBUG -D_WINDLL -mdll -o portmidi.dll -Wl,--out-implib,libportmidi.a pm_win/pmwin.c pm_win/pmwinmm.c pm_common/pmutil.c pm_common/portmidi.c -L. -lporttime -lwinmm
  cp -v portmidi.dll porttime.dll "$PREFIX"/bin
  cp -v libportmidi.a libporttime.a "$PREFIX"/lib
  cp -v pm_common/portmidi.h porttime/porttime.h "$PREFIX"/include
  cd ..
  touch "$PREFIX"/build-stamps/portmidi
  $RM_RF portmidi
fi

# SDL
SDL="SDL-1.2.14"
if test ! -f "$PREFIX"/build-stamps/sdl; then
  download http://www.libsdl.org/release/$SDL.tar.gz
  tar zxvf $SDL.tar.gz
  cd $SDL
  ./configure $COMMON_AUTOCONF_FLAGS --disable-stdio-redirect
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/sdl
  $RM_RF $SDL
fi

# Freetype
FREETYPE="freetype-2.4.3"
if test ! -f "$PREFIX"/build-stamps/freetype; then
  download http://download.sourceforge.net/freetype/$FREETYPE.tar.bz2
  tar jxvf $FREETYPE.tar.bz2
  cd $FREETYPE
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/freetype
  $RM_RF $FREETYPE
fi

# GLib
GLIB="glib-2.26.0"
if test ! -f "$PREFIX"/build-stamps/glib; then
  download http://ftp.gnome.org/pub/GNOME/sources/glib/2.26/$GLIB.tar.bz2
  tar jxvf $GLIB.tar.bz2
  cd $GLIB
  ./configure $COMMON_AUTOCONF_FLAGS
  make -C glib
  make -C gthread
  make -C gobject glib-genmarshal.exe
  wine-shwrap gobject/glib-genmarshal
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/glib
  $RM_RF $GLIB
fi

# Fontconfig
FONTCONFIG="fontconfig-2.8.0"
if test ! -f "$PREFIX"/build-stamps/fontconfig; then
  download http://fontconfig.org/release/$FONTCONFIG.tar.gz
  tar zxvf $FONTCONFIG.tar.gz
  cd $FONTCONFIG
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  # It tries to build its initial cache by invoking the installed binary, so make it happy.
  wine-shwrap "$PREFIX"/bin/fc-cache
  make install
  rm -f "$PREFIX"/bin/fc-cache
  cd ..
  touch "$PREFIX"/build-stamps/fontconfig
  $RM_RF $FONTCONFIG
fi

# Pixman
PIXMAN="pixman-0.19.6"
if test ! -f "$PREFIX"/build-stamps/pixman; then
  download http://cairographics.org/releases/$PIXMAN.tar.gz
  tar zxvf $PIXMAN.tar.gz
  cd $PIXMAN
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/pixman
  $RM_RF $PIXMAN
fi

# Cairo
CAIRO="cairo-1.8.10"
if test ! -f "$PREFIX"/build-stamps/cairo; then
  download http://cairographics.org/releases/$CAIRO.tar.gz
  tar zxvf $CAIRO.tar.gz
  cd $CAIRO
  ./configure $COMMON_AUTOCONF_FLAGS --disable-xlib
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/cairo
  $RM_RF $CAIRO
fi

# Pango
# For some reason, the libraries are installed in an extremely uncharacteristic
# layout for libtool.  Luckily, we have all that we need to make it sane again.
PANGO="pango-1.28.3"
if test ! -f "$PREFIX"/build-stamps/pango; then
  download http://ftp.gnome.org/pub/GNOME/sources/pango/1.28/$PANGO.tar.bz2
  tar jxvf $PANGO.tar.bz2
  cd $PANGO
  ./configure $COMMON_AUTOCONF_FLAGS --with-included-modules=yes --with-dynamic-modules=no CXX="$CROSS_GXX \"-D__declspec(x)=__attribute__((x))\""
  make
  make install
  for f in '' cairo ft2 win32; do
    mv "$PREFIX"/lib/libpango$f-1.0-0.dll "$PREFIX"/bin
    rm -f "$PREFIX"/lib/libpango$f-1.0.lib
    $CROSS_DLLTOOL -D libpango$f-1.0-0.dll -d "$PREFIX"/lib/pango$f-1.0.def -l "$PREFIX"/lib/libpango$f-1.0.a
    sed -i -e "s/libpango$f-1.0.lib/libpango$f-1.0.a/g" "$PREFIX"/lib/libpango$f-1.0.la
  done
  cd ..
  touch "$PREFIX"/build-stamps/pango
  $RM_RF $PANGO
fi

# ATK
# We don't really need to build the parts of GTK that use ATK, but GTK's
# configure script won't be happy otherwise.
ATK="atk-1.32.0"
if test ! -f "$PREFIX"/build-stamps/atk; then
  download http://ftp.gnome.org/pub/GNOME/sources/atk/1.32/$ATK.tar.bz2
  tar jxvf $ATK.tar.bz2
  cd $ATK
  ./configure $COMMON_AUTOCONF_FLAGS --disable-glibtest
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/atk
  $RM_RF $ATK
fi

# Gdk-pixbuf
# New versions of GTK require this separately
GDKPIXBUF="gdk-pixbuf-2.22.0"
if test ! -f "$PREFIX"/build-stamps/gdkpixbuf; then
  download http://ftp.gnome.org/pub/GNOME/sources/gdk-pixbuf/2.22/$GDKPIXBUF.tar.bz2
  tar jxvf $GDKPIXBUF.tar.bz2
  cd $GDKPIXBUF
  ./configure $COMMON_AUTOCONF_FLAGS --without-libtiff
  make
  wine-shwrap gdk-pixbuf/gdk-pixbuf-csource
  wine-shwrap gdk-pixbuf/gdk-pixbuf-query-loaders
  make install
  cd ..
  touch "$PREFIX"/build-stamps/gdkpixbuf
  $RM_RF $ATK
fi

# GTK
# Even though Performous only needs libgdk_pixbuf (and then only because
# that's how librsvg likes to play ball), having the rest of GTK around
# lets other packages build some handy test programs that can be run
# under Wine to sanity-check the dependency build.  The choice of included
# pixbuf loaders is tailored to satisfy said programs.
GTK="gtk+-2.22.0"
if test ! -f "$PREFIX"/build-stamps/gtk; then
  download http://ftp.gnome.org/pub/GNOME/sources/gtk+/2.22/$GTK.tar.bz2
  tar jxvf $GTK.tar.bz2
  cd $GTK
  ./configure $COMMON_AUTOCONF_FLAGS --disable-glibtest --disable-modules --without-libtiff --with-included-loaders=png,jpeg,gif,xpm,xbm
  make -C gdk
  make -C gtk gtk-update-icon-cache.exe
  wine-shwrap gtk/gtk-update-icon-cache
  make -C gtk gtk-query-immodules-2.0.exe
  wine-shwrap gtk/gtk-query-immodules-2.0
  make
  make install
  find "$PREFIX"/lib/gtk-2.0 -name '*.la' -print0 | xargs -0 rm -f
  find "$PREFIX"/lib/gtk-2.0 -name '*.dll.a' -print0 | xargs -0 rm -f
  # Use the Wimp (native Windows widgets) theme.
  cp "$PREFIX"/share/themes/MS-Windows/gtk-2.0/gtkrc "$PREFIX"/etc/gtk-2.0
  cd ..
  touch "$PREFIX"/build-stamps/gtk
  $RM_RF $GTK
fi

# libcroco
LIBCROCO="libcroco-0.6.2"
if test ! -f "$PREFIX"/build-stamps/libcroco; then
  download http://ftp.gnome.org/pub/GNOME/sources/libcroco/0.6/$LIBCROCO.tar.bz2
  tar jxvf $LIBCROCO.tar.bz2
  cd $LIBCROCO
  # We bypass a file magick check here
  lt_cv_deplibs_check_method=pass_all ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libcroco
  $RM_RF $LIBCROCO
fi

# libgsf
# It will try to install a gconf schema on the host unless we tell it not to,
# despite the fact that it doesn't have the GNOME libraries it would need to
# make any use of it...
LIBGSF="libgsf-1.14.19"
if test ! -f "$PREFIX"/build-stamps/libgsf; then
  download http://ftp.gnome.org/pub/GNOME/sources/libgsf/1.14/$LIBGSF.tar.bz2
  tar jxvf $LIBGSF.tar.bz2
  cd $LIBGSF
  ./configure $COMMON_AUTOCONF_FLAGS --without-python --disable-schemas-install
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libgsf
  $RM_RF $LIBGSF
fi

# librsvg
LIBRSVG="librsvg-2.32.0"
if test ! -f "$PREFIX"/build-stamps/librsvg; then
  download http://ftp.gnome.org/pub/GNOME/sources/librsvg/2.32/$LIBRSVG.tar.bz2
  tar jxvf $LIBRSVG.tar.bz2
  cd $LIBRSVG
  ./configure $COMMON_AUTOCONF_FLAGS --disable-pixbuf-loader --disable-gtk-theme
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/librsvg
  $RM_RF $LIBRSVG
fi

# libsigc++
LIBSIGCPP="libsigc++-2.2.8"
if test ! -f "$PREFIX"/build-stamps/libsigc++; then
  download http://ftp.gnome.org/pub/GNOME/sources/libsigc++/2.2/$LIBSIGCPP.tar.bz2
  tar jxvf $LIBSIGCPP.tar.bz2
  cd $LIBSIGCPP
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libsigc++
  $RM_RF $LIBSIGCPP
fi

# libglibmm
GLIBMM="glibmm-2.25.2"
if test ! -f "$PREFIX"/build-stamps/libglibmm; then
  download http://ftp.gnome.org/pub/GNOME/sources/glibmm/2.25/$GLIBMM.tar.bz2
  tar jxvf $GLIBMM.tar.bz2
  cd $GLIBMM
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libglibmm
  $RM_RF $GLIBMM
fi

# libxml++
LIBXMLPP="libxml++-2.32.0"
if test ! -f "$PREFIX"/build-stamps/libxml++; then
  download http://ftp.gnome.org/pub/GNOME/sources/libxml++/2.32/$LIBXMLPP.tar.bz2
  tar jxvf $LIBXMLPP.tar.bz2
  cd $LIBXMLPP
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libxml++
  $RM_RF $LIBXMLPP
fi

# GLEW, which lacks a build system amenable to cross-compilation.
# glewinfo and visualinfo are handy if you ever need something
# similar to glxinfo under Windows.
GLEW="glew-1.5.6"
if test ! -f "$PREFIX"/build-stamps/glew; then
  download http://download.sourceforge.net/glew/$GLEW.tgz
  tar zxvf $GLEW.tgz
  cd $GLEW
  $CROSS_WINDRES -o glewres.o build/vc6/glew.rc
  $CROSS_GCC -g -O2 -W -Wall -Iinclude -DGLEW_BUILD -mdll -Wl,--out-implib,libGLEW.a -o glew32.dll src/glew.c glewres.o -lopengl32 -lglu32 -lgdi32
  $CROSS_WINDRES -o glewinfores.o build/vc6/glewinfo.rc
  $CROSS_GCC -g -O2 -W -Wall -Iinclude -o glewinfo.exe src/glewinfo.c glewinfores.o -L. -lGLEW -lopengl32 -lglu32 -lgdi32
  $CROSS_WINDRES -o visualinfores.o build/vc6/visualinfo.rc
  $CROSS_GCC -g -O2 -W -Wall -Iinclude -o visualinfo.exe src/visualinfo.c visualinfores.o -L. -lGLEW -lopengl32 -lglu32 -lgdi32
  make GLEW_DEST="$PREFIX" glew.pc
  cp -v glew32.dll glewinfo.exe visualinfo.exe "$PREFIX"/bin
  cp -v libGLEW.a "$PREFIX"/lib
  mkdir -pv "$PREFIX"/include/GL "$PREFIX"/lib/pkgconfig
  cp -v include/GL/* "$PREFIX"/include/GL
  cp -v glew.pc "$PREFIX"/lib/pkgconfig
  cd ..
  touch "$PREFIX"/build-stamps/glew
  $RM_RF $GLEW
fi

# ImageMagick
# Needed for tools (ss_extract) only.
IM="ImageMagick-6.6.5-5"
if test ! -f "$PREFIX"/build-stamps/imagemagick; then
  download http://image_magick.veidrodis.com/image_magick/ImageMagick-6.6.5-5.tar.bz2
  tar jxvf $IM.tar.bz2
  cd $IM
  ./configure $COMMON_AUTOCONF_FLAGS --without-rsvg --without-gslib --without-gvc --without-wmf --without-frozenpaths --without-x
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/imagemagick
  $RM_RF $IM
fi

# liborc
LIBORC="orc-0.4.10"
if test ! -f "$PREFIX"/build-stamps/liborc; then
  download http://code.entropywave.com/download/orc/$LIBORC.tar.gz
  tar zxvf $LIBORC.tar.gz
  cd $LIBORC
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/liborc
  $RM_RF $LIBORC
fi

# libschroedinger
LIBSCHROE="schroedinger-1.0.10"
if test ! -f "$PREFIX"/build-stamps/libschroedinger; then
  download http://diracvideo.org/download/schroedinger/$LIBSCHROE.tar.gz
  tar zxvf $LIBSCHROE.tar.gz
  cd $LIBSCHROE
  ./configure $COMMON_AUTOCONF_FLAGS
  # We don't want testsuite, so create a dummy Makefile
  echo "all:\n\ninstall:" > testsuite/Makefile
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libschroedinger
  $RM_RF $LIBSCHROE
fi

# FFmpeg
# The only external library I have gone out of my way to add is
# libschroedinger because it needs it to decode Dirac, and not having
# at least decode support for all free formats would be sad.  (Speaking of
# free format support, I skipped the Xiph libs and libvpx because they're
# only needed for *encoding* the relevant formats; ffmpeg's native VP8
# (WebM) decoding support is suboptimal but passable at the time of this
# writing and can only improve from here, and since we use latest SVN at
# time of build (as the ffmpeg developers suggest), we get the improvements
# automatically with a dep rebuild.)
if test ! -f "$PREFIX"/build-stamps/ffmpeg; then
  if test ! -d ffmpeg; then
    svn co svn://svn.ffmpeg.org/ffmpeg/trunk ffmpeg
  else
    svn up ffmpeg
  fi
  cd ffmpeg
  ./configure --prefix="$PREFIX" --cc="$CROSS_GCC" --nm="$CROSS_NM" --target-os=mingw32 --arch=i386 --disable-static --enable-shared --enable-gpl --enable-postproc --enable-w32threads --enable-runtime-cpudetect --enable-memalign-hack --enable-zlib --enable-libschroedinger --extra-cflags="-I$PREFIX/include" --extra-ldflags="-L$PREFIX/lib"
  sed -i -e 's/-Werror=[^ ]*//g' config.mak
  make
  make install
  for lib in avcodec avdevice avfilter avformat avutil postproc swscale; do
    # FFmpeg symlinks its DLLs to a few different names, differing in the level
    # of detail of their version number, rather like what is done with ELF shared
    # libraries.  Unfortunately, the real DLL for each one is *not* the one that
    # the implibs reference (that is, the one that will be required at runtime),
    # so we must rename it after we nuke the symlinks.
    find "$PREFIX"/bin -type l -name "${lib}*.dll" -print0 | xargs -0 rm -f
    libfile="`find "$PREFIX"/bin -name "${lib}*.dll" | sed -e 1q`"
    mv -v "$libfile" "`echo "$libfile" | sed -e "s/\($lib-[0-9]*\)[.0-9]*\.dll/\1.dll/"`"
  done
  cd ..
  touch "$PREFIX"/build-stamps/ffmpeg
  $RM_RF ffmpeg
fi

# VideoInput
# This is support library for OpenCV to get video input on Windows.
# This is only needed if you want OpenCV.
VIDEOINPUT="videoInput0.1995"
if test ! -f "$PREFIX"/build-stamps/videoinput; then
  download http://muonics.net/school/spring05/videoInput/files/$VIDEOINPUT.zip
  unzip -o $VIDEOINPUT.zip
  # We don't bother compiling this since it has precompiled lib and would
  # probably be a pain in the ass.
  cp -v $VIDEOINPUT/compiledLib/compiledByDevCpp/*.a "$PREFIX"/lib
  cp -v $VIDEOINPUT/compiledLib/compiledByDevCpp/include/*.h "$PREFIX"/include
  touch "$PREFIX"/build-stamps/videoinput
  $RM_RF $VIDEOINPUT
fi

# OpenCV
# This is optional and is used to enable webcam features.
OPENCV="OpenCV-2.1.0"
if test ! -f "$PREFIX"/build-stamps/opencv; then
  download http://download.sourceforge.net/opencvlibrary/$OPENCV-win.zip
  unzip -o $OPENCV-win.zip
  cd $OPENCV
  ln -s "$PREFIX/include/videoInput.h" src/highgui/videoinput.h
  mkdir -pv build
  cd build
  cmake \
    -DCMAKE_TOOLCHAIN_FILE="$SCRIPTDIR/Toolchain.cmake" \
	-DBUILD_NEW_PYTHON_SUPPORT=OFF \
	-DBUILD_TESTS=OFF \
	-DWITH_JASPER=OFF -DWITH_JPEG=OFF -DWITH_PNG=OFF -DWITH_TBB=OFF -DWITH_TIFF=OFF \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    ..
  make
  make install
  cp -v unix-install/opencv.pc "$PREFIX"/lib/pkgconfig  # No auto-install :(
  cd "$PREFIX"/lib
  # Create some symlinks so the cmake scripts detect it correctly
  ln -svf libcv210.dll.a libcv.dll.a
  ln -svf libcxcore210.dll.a libcxcore.dll.a
  ln -svf libcvaux210.dll.a libcvaux.dll.a
  ln -svf libhighgui210.dll.a libhighgui.dll.a
  cd "$PREFIX/.."
  touch "$PREFIX"/build-stamps/opencv
  $RM_RF $OPENCV
fi

echo "All dependencies done."
