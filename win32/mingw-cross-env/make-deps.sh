#!/bin/bash -e

if [ ! "$1" ]; then
	echo "Usage: $0 prefix-path"
	exit 1
fi

prefix="$1"

# Create the prefix path and copy our own suplementary scripts
mkdir -p "$prefix/src"
cp mk/*.mk "$prefix/src"

# Download the cross env if needed
if [ ! -f "$prefix/Makefile" ]; then
	crossenvpackage=mingw-cross-env-2.15
	cd /tmp
	wget http://download.savannah.nongnu.org/releases/mingw-cross-env/$crossenvpackage.tar.gz
	tar zxf $crossenvpackage.tar.gz
	cp -r $crossenvpackage/* "$prefix"
	rm -rf $crossenvpackage
fi

cd "$prefix"

echo "Make gcc"
make gcc

echo "Make deps"
make zlib libpng jpeg glew freeglut sdl boost atk cairo fontconfig freetype gettext glib gtk libcroco libgsf libiconv librsvg libxml2 pango pixman portaudio libsigc++ glibmm libxml++ liborc libschroedinger

echo "Make ffmpeg from SVN"
TARGET=i686-pc-mingw32
export PATH="$prefix/usr/bin:$PATH"
export PKG_CONFIG_PATH=$prefix/usr/i686-pc-mingw32/lib/pkgconfig/

# Checkout / update from svn
if [ ! -d ffmpeg ]; then
	svn co svn://svn.ffmpeg.org/ffmpeg/trunk ffmpeg
else
	svn up ffmpeg
fi

cd ffmpeg
	./configure --prefix="$prefix/usr/$TARGET" --enable-cross-compile --cross-prefix="$TARGET-" --target-os=mingw32 --arch=i386 --disable-shared --enable-static --enable-gpl --enable-postproc --enable-w32threads --enable-runtime-cpudetect --enable-memalign-hack --enable-zlib --enable-libschroedinger --disable-muxers --disable-encoders --disable-ffserver --disable-ffplay --disable-protocol=http --disable-protocol=rtp --disable-protocol=tcp --disable-protocol=udp --extra-cflags="-I$prefix/usr/$TARGET/include" --extra-ldflags="-L$prefix/usr/$TARGET/lib"
	sed -i -e 's/-Werror=[^ ]*//g' config.mak
	make -j2
	make install
cd ..

echo "All done"
