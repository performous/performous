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
if test ! -f "$PREFIX"/build-stamps/win-iconv; then
  download http://win-iconv.googlecode.com/files/win-iconv-0.0.1.tar.bz2
  tar jxvf win-iconv-0.0.1.tar.bz2
  cd win-iconv-0.0.1
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
  $RM_RF win-iconv-0.0.1
fi

# zlib
if test ! -f "$PREFIX"/build-stamps/zlib; then
  download http://www.zlib.net/zlib-1.2.5.tar.bz2
  tar jxvf zlib-1.2.5.tar.bz2
  cd zlib-1.2.5
  make -f win32/Makefile.gcc PREFIX="$CROSS_TOOL_PREFIX"- zlib1.dll
  cp -v zlib.h zconf.h "$PREFIX"/include
  cp -v zlib1.dll "$PREFIX"/bin
  cp -v libzdll.a "$PREFIX"/lib/libz.a
  cd ..
  touch "$PREFIX"/build-stamps/zlib
  $RM_RF zlib-1.2.5
fi

# libpng
if test ! -f "$PREFIX"/build-stamps/libpng; then
  download http://download.sourceforge.net/libpng/libpng-1.4.2.tar.gz
  tar zxvf libpng-1.4.2.tar.gz
  cd libpng-1.4.2
  make -f scripts/makefile.mingw prefix="$PREFIX" CC="$CROSS_GCC" AR="$CROSS_AR" RANLIB="$CROSS_RANLIB" ZLIBINC="$PREFIX"/include ZLIBLIB="$PREFIX"/lib install-shared
  cd ..
  touch "$PREFIX"/build-stamps/libpng
  $RM_RF libpng-1.4.2
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
if test ! -f "$PREFIX"/build-stamps/gettext-runtime; then
  download http://ftp.gnu.org/gnu/gettext/gettext-0.18.tar.gz
  tar zxvf gettext-0.18.tar.gz
  cd gettext-0.18/gettext-runtime
  ./configure $COMMON_AUTOCONF_FLAGS --enable-relocatable --disable-libasprintf --disable-java --disable-csharp
  make
  make install
  cd ../..
  touch "$PREFIX"/build-stamps/gettext-runtime
  $RM_RF gettext-0.18
fi

# libxml2
if test ! -f "$PREFIX"/build-stamps/libxml2; then
  download ftp://xmlsoft.org/libxml2/libxml2-2.7.7.tar.gz
  tar zxvf libxml2-2.7.7.tar.gz
  cd libxml2-2.7.7
  ./configure $COMMON_AUTOCONF_FLAGS --without-python --without-readline
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libxml2
  $RM_RF libxml2-2.7.7
fi

# The bits of Boost that we need.
if test ! -f "$PREFIX"/build-stamps/boost; then
  download http://download.sourceforge.net/boost/boost_1_43_0.tar.bz2
  tar jxvf boost_1_43_0.tar.bz2
  cd boost_1_43_0/tools/jam/src
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
  $RM_RF boost_1_43_0
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
  $CROSS_GCC -g -O2 -W -Wall -Ipm_common -Iporttime -DNDEBUG -D_WINDLL -mdll -o portmidi.dll -Wl,--out-implib,libportmidi.a pm_win/pmwin.c pm_win/pmwinmm.c porttime/ptwinmm.c pm_common/pmutil.c pm_common/portmidi.c -lwinmm
  cp -v portmidi.dll "$PREFIX"/bin
  cp -v libportmidi.a "$PREFIX"/lib
  cp -v pm_common/portmidi.h porttime/porttime.h "$PREFIX"/include
  cd ..
  touch "$PREFIX"/build-stamps/portmidi
  $RM_RF portmidi
fi

# SDL
if test ! -f "$PREFIX"/build-stamps/sdl; then
  download http://www.libsdl.org/release/SDL-1.2.14.tar.gz
  tar zxvf SDL-1.2.14.tar.gz
  cd SDL-1.2.14
  ./configure $COMMON_AUTOCONF_FLAGS --disable-stdio-redirect
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/sdl
  $RM_RF SDL-1.2.14
fi

# Freetype
if test ! -f "$PREFIX"/build-stamps/freetype; then
  download http://download.sourceforge.net/freetype/freetype-2.3.12.tar.bz2
  tar jxvf freetype-2.3.12.tar.bz2
  cd freetype-2.3.12
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/freetype
  $RM_RF freetype-2.3.12
fi

# GLib
if test ! -f "$PREFIX"/build-stamps/glib; then
  download http://ftp.gnome.org/pub/GNOME/sources/glib/2.24/glib-2.24.1.tar.bz2
  tar jxvf glib-2.24.1.tar.bz2
  cd glib-2.24.1
  ./configure $COMMON_AUTOCONF_FLAGS
  make -C glib
  make -C gthread
  make -C gobject glib-genmarshal.exe
  wine-shwrap gobject/glib-genmarshal
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/glib
  $RM_RF glib-2.24.1
fi

# Fontconfig
if test ! -f "$PREFIX"/build-stamps/fontconfig; then
  download http://fontconfig.org/release/fontconfig-2.8.0.tar.gz
  tar zxvf fontconfig-2.8.0.tar.gz
  cd fontconfig-2.8.0
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  # It tries to build its initial cache by invoking the installed binary, so make it happy.
  wine-shwrap "$PREFIX"/bin/fc-cache
  make install
  rm -f "$PREFIX"/bin/fc-cache
  cd ..
  touch "$PREFIX"/build-stamps/fontconfig
  $RM_RF fontconfig-2.8.0
fi

# Pixman
if test ! -f "$PREFIX"/build-stamps/pixman; then
  download http://cairographics.org/releases/pixman-0.18.2.tar.gz
  tar zxvf pixman-0.18.2.tar.gz
  cd pixman-0.18.2
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/pixman
  $RM_RF pixman-0.18.2
fi

# Cairo
if test ! -f "$PREFIX"/build-stamps/cairo; then
  download http://cairographics.org/releases/cairo-1.8.10.tar.gz
  tar zxvf cairo-1.8.10.tar.gz
  cd cairo-1.8.10
  ./configure $COMMON_AUTOCONF_FLAGS --disable-xlib
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/cairo
  $RM_RF cairo-1.8.10
fi

# Pango
# For some reason, the libraries are installed in an extremely uncharacteristic
# layout for libtool.  Luckily, we have all that we need to make it sane again.
if test ! -f "$PREFIX"/build-stamps/pango; then
  download http://ftp.gnome.org/pub/GNOME/sources/pango/1.28/pango-1.28.1.tar.bz2
  tar jxvf pango-1.28.1.tar.bz2
  cd pango-1.28.1
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
  $RM_RF pango-1.28.1
fi

# ATK
# We don't really need to build the parts of GTK that use ATK, but GTK's
# configure script won't be happy otherwise.
if test ! -f "$PREFIX"/build-stamps/atk; then
  download http://ftp.gnome.org/pub/GNOME/sources/atk/1.30/atk-1.30.0.tar.bz2
  tar jxvf atk-1.30.0.tar.bz2
  cd atk-1.30.0
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/atk
  $RM_RF atk-1.30.0
fi

# GTK
# Even though Performous only needs libgdk_pixbuf (and then only because
# that's how librsvg likes to play ball), having the rest of GTK around
# lets other packages build some handy test programs that can be run
# under Wine to sanity-check the dependency build.  The choice of included
# pixbuf loaders is tailored to satisfy said programs.
if test ! -f "$PREFIX"/build-stamps/gtk; then
  download http://ftp.gnome.org/pub/GNOME/sources/gtk+/2.20/gtk+-2.20.1.tar.bz2
  tar jxvf gtk+-2.20.1.tar.bz2
  cd gtk+-2.20.1
  ./configure $COMMON_AUTOCONF_FLAGS --disable-modules --without-libtiff --with-included-loaders=png,jpeg,gif,xpm,xbm
  make -C gdk-pixbuf
  wine-shwrap gdk-pixbuf/gdk-pixbuf-csource
  wine-shwrap gdk-pixbuf/gdk-pixbuf-query-loaders
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
  $RM_RF gtk+-2.20.1
fi

# libcroco
if test ! -f "$PREFIX"/build-stamps/libcroco; then
  download http://ftp.gnome.org/pub/GNOME/sources/libcroco/0.6/libcroco-0.6.2.tar.bz2
  tar jxvf libcroco-0.6.2.tar.bz2
  cd libcroco-0.6.2
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libcroco
  $RM_RF libcroco-0.6.2
fi

# libgsf
# It will try to install a gconf schema on the host unless we tell it not to,
# despite the fact that it doesn't have the GNOME libraries it would need to
# make any use of it...
if test ! -f "$PREFIX"/build-stamps/libgsf; then
  download http://ftp.gnome.org/pub/GNOME/sources/libgsf/1.14/libgsf-1.14.18.tar.bz2
  tar jxvf libgsf-1.14.18.tar.bz2
  cd libgsf-1.14.18
  ./configure $COMMON_AUTOCONF_FLAGS --without-python --disable-schemas-install
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libgsf
  $RM_RF libgsf-1.14.18
fi

# librsvg
if test ! -f "$PREFIX"/build-stamps/librsvg; then
  download http://ftp.gnome.org/pub/GNOME/sources/librsvg/2.26/librsvg-2.26.3.tar.bz2
  tar jxvf librsvg-2.26.3.tar.bz2
  cd librsvg-2.26.3
  ./configure $COMMON_AUTOCONF_FLAGS --disable-pixbuf-loader --disable-gtk-theme
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/librsvg
  $RM_RF librsvg-2.26.3
fi

# libsigc++
if test ! -f "$PREFIX"/build-stamps/libsigc++; then
  download http://ftp.gnome.org/pub/GNOME/sources/libsigc++/2.2/libsigc++-2.2.8.tar.bz2
  tar jxvf libsigc++-2.2.8.tar.bz2
  cd libsigc++-2.2.8
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libsigc++
  $RM_RF libsigc++-2.2.8
fi

# libglibmm
if test ! -f "$PREFIX"/build-stamps/libglibmm; then
  download http://ftp.gnome.org/pub/GNOME/sources/glibmm/2.24/glibmm-2.24.2.tar.bz2
  tar jxvf glibmm-2.24.2.tar.bz2
  cd glibmm-2.24.2
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libglibmm
  $RM_RF glibmm-2.24.2
fi

# libxml++
if test ! -f "$PREFIX"/build-stamps/libxml++; then
  download http://ftp.gnome.org/pub/GNOME/sources/libxml++/2.30/libxml++-2.30.1.tar.bz2
  tar jxvf libxml++-2.30.1.tar.bz2
  cd libxml++-2.30.1
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libxml++
  $RM_RF libxml++-2.30.1
fi

# GLEW, which lacks a build system amenable to cross-compilation.
# glewinfo and visualinfo are handy if you ever need something
# similar to glxinfo under Windows.
if test ! -f "$PREFIX"/build-stamps/glew; then
  download http://download.sourceforge.net/glew/glew-1.5.4.tgz
  tar zxvf glew-1.5.4.tgz
  cd glew-1.5.4
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
  $RM_RF glew-1.5.4
fi

# liborc
if test ! -f "$PREFIX"/build-stamps/liborc; then
  download http://code.entropywave.com/download/orc/orc-0.4.5.tar.gz
  tar zxvf orc-0.4.5.tar.gz
  cd orc-0.4.5
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/liborc
  $RM_RF orc-0.4.5
fi

# libschroedinger
if test ! -f "$PREFIX"/build-stamps/libschroedinger; then
  download http://diracvideo.org/download/schroedinger/schroedinger-1.0.9.tar.gz
  tar zxvf schroedinger-1.0.9.tar.gz
  cd schroedinger-1.0.9
  ./configure $COMMON_AUTOCONF_FLAGS
  make
  make install
  cd ..
  touch "$PREFIX"/build-stamps/libschroedinger
  $RM_RF schroedinger-1.0.9
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
  ./configure --prefix="$PREFIX" --cc="$CROSS_GCC" --nm="$CROSS_NM" --target-os=mingw32 --arch=i386 --disable-static --enable-shared --enable-gpl --enable-postproc --enable-avfilter-lavf --enable-w32threads --enable-runtime-cpudetect --enable-memalign-hack --enable-zlib --enable-libschroedinger --extra-cflags="-I$PREFIX/include" --extra-ldflags="-L$PREFIX/lib"
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
