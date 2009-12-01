#!/bin/sh 

#The root of you build directory for dependencies
BUILD_DIR=/build

#Internal script variable
INSTALL_DIR=

#Each time we install a library, add the necessary paths
add_paths(){
	PATH=$PATH:$BUILD_DIR/$INSTALL_DIR/bin
	CPATH=$CPATH:$BUILD_DIR/$INSTALL_DIR/include
	LIBRARY_PATH=$LIBRARY_PATH:$BUILD_DIR/$INSTALL_DIR/lib
	PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$BUILD_DIR/$INSTALL_DIR/lib/pkgconfig
}

download(){
	filename=$1
	filename=${filename##*/}
	if ! test -e filename #file not exists, download it
	then
		wget $1
	fi
}

#Resolve the libstdc++.dll problem
patch -p0 < mingw-libstdc++.patch
patch -p0 < mingw-cwchar.patch

#Install the pkg-config pre-built
cp pkg-config.exe /mingw/bin/pkg-config.exe


#****************
#***   zlib   ***
#****************
download http://www.zlib.net/zlib-1.2.3.tar.gz
tar xzf zlib-1.2.3.tar.gz
cd zlib-1.2.3
	INSTALL_DIR=zlib-1.2.3
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	mkdir -p $BUILD_DIR/$INSTALL_DIR/bin
	gcc -shared -o $BUILD_DIR/$INSTALL_DIR/bin/zlib1.dll -Wl,--out-implib=$BUILD_DIR/$INSTALL_DIR/lib/libz.dll.a [!em]*.o
	add_paths
cd ..

#****************
#**  libiconv  **
#****************
download http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.13.1.tar.gz
tar xzf libiconv-1.13.1.tar.gz
cd libiconv-1.13.1
	INSTALL_DIR=libiconv-1.13.1
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*****************
#***  gettext  ***
#*****************
download http://ftp.gnu.org/pub/gnu/gettext/gettext-0.17.tar.gz
tar xzf gettext-0.17.tar.gz
cd gettext-0.17
	INSTALL_DIR=gettext-0.17
	
	patch -p0 < ../gettext-0.17-variables.patch
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --enable-threads=win32 --enable-relocatable && make && make install
	add_paths
cd ..

#******************
#****   glib   ****
#******************
download http://ftp.gnome.org/pub/gnome/sources/glib/2.22/glib-2.22.2.tar.gz
tar xzf glib-2.22.2.tar.gz
cd glib-2.22.2
	INSTALL_DIR=glib-2.22.2
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --with-threads=win32 --with-pcre=internal && make && make install
	add_paths
cd ..

#******************
#****  libpng  ****
#******************
download http://freefr.dl.sourceforge.net/project/libpng/00-libpng-stable/1.2.40/libpng-1.2.40.tar.gz
tar xzf libpng-1.2.40.tar.gz
cd libpng-1.2.40
	INSTALL_DIR=libpng-1.2.40
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*******************
#****  libjpeg  ****
#*******************
download http://www.ijg.org/files/jpegsrc.v7.tar.gz
tar xzf jpegsrc.v7.tar.gz
cd jpeg-7
	INSTALL_DIR=jpeg-7
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*******************
#****  libtiff  ****
#*******************
download ftp://ftp.remotesensing.org/pub/libtiff/tiff-3.9.1.tar.gz
tar xzf tiff-3.9.1.tar.gz
cd tiff-3.9.1
	INSTALL_DIR=tiff-3.9.1
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#********************
#****  freetype  ****
#********************
download http://ignum.dl.sourceforge.net/project/freetype/freetype2/2.3.11/freetype-2.3.11.tar.gz
tar xzf freetype-2.3.11.tar.gz
cd freetype-2.3.11
	INSTALL_DIR=freetype-2.3.11
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#******************
#****  libxml  ****
#******************
# Don't use last version, seems broken. This is the last that works with libxml++
download ftp://xmlsoft.org/libxml2/libxml2-2.7.3.tar.gz
tar xzf libxml2-2.7.3.tar.gz
cd libxml2-2.7.3
	INSTALL_DIR=libxml2-2.7.3
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --without-threads && make && make install
	add_paths
cd ..

#********************
#***  fontconfig  ***
#********************
download http://fontconfig.org/release/fontconfig-2.8.0.tar.gz
tar xzf fontconfig-2.8.0.tar.gz
cd fontconfig-2.8.0
	INSTALL_DIR=fontconfig-2.8.0
	
	mkdir $BUILD_DIR/$INSTALL_DIR/lib
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --enable-libxml2 && make && make install
	add_paths
cd ..

#*******************
#*****   atk   *****
#*******************
download http://ftp.gnome.org/pub/gnome/sources/atk/1.29/atk-1.29.2.tar.gz
tar xzf atk-1.29.2.tar.gz
cd atk-1.29.2
	INSTALL_DIR=atk-1.29.2
	
	mkdir $BUILD_DIR/$INSTALL_DIR/lib
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#******************
#****  pixman  ****
#******************
download http://cairographics.org/releases/pixman-0.17.2.tar.gz
tar xzf pixman-0.17.2.tar.gz
cd pixman-0.17.2
	INSTALL_DIR=pixman-0.17.2
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*****************
#****  cairo  ****
#*****************
download http://cairographics.org/releases/cairo-1.8.8.tar.gz
tar xzf cairo-1.8.8.tar.gz
cd cairo-1.8.8
	INSTALL_DIR=cairo-1.8.8
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --disable-pthread && make && make install
	add_paths
cd ..

#*****************
#****  pango  ****
#*****************
download http://ftp.gnome.org/pub/GNOME/sources/pango/1.26/pango-1.26.1.tar.gz
tar xzf pango-1.26.1.tar.gz
cd pango-1.26.1
	INSTALL_DIR=pango-1.26.1
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --with-included-modules=yes && make && make install
	add_paths
cd ..

#****************
#****  gtk+  ****
#****************
download http://ftp.gnome.org/pub/gnome/sources/gtk+/2.19/gtk+-2.19.0.tar.gz
tar xzf gtk+-2.19.0.tar.gz
cd gtk+-2.19.0
	INSTALL_DIR=gtk+-2.19.0
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR --with-gdktarget=win32 --disable-gdiplus --with-included-immodules --without-libjasper --enable-debug=yes --enable-explicit-deps=no --disable-gtk-doc --disable-static && make && make install
	add_paths
cd ..

#*******************
#****  librsvg  ****
#*******************
download http://ftp.gnome.org/pub/GNOME/sources/librsvg/2.26/librsvg-2.26.0.tar.gz
tar xzf librsvg-2.26.0.tar.gz
cd librsvg-2.26.0
	INSTALL_DIR=librsvg-2.26.0
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*******************
#******  sdl  ******
#*******************
download http://www.libsdl.org/release/SDL-1.2.14.tar.gz
tar xzf SDL-1.2.14.tar.gz
cd SDL-1.2.14
	INSTALL_DIR=SDL-1.2.14
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*******************
#***  libsigc++  ***
#*******************
download http://ftp.gnome.org/pub/GNOME/sources/libsigc++/2.2/libsigc++-2.2.4.tar.gz
tar xzf libsigc++-2.2.4.tar.gz
cd libsigc++-2.2.4
	INSTALL_DIR=libsigc++-2.2.4
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#******************
#****  glibmm  ****
#******************
download http://ftp.gnome.org/pub/GNOME/sources/glibmm/2.22/glibmm-2.22.1.tar.gz
tar xzf glibmm-2.22.1.tar.gz
cd glibmm-2.22.1
	INSTALL_DIR=glibmm-2.22.1
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#******************
#***  libxml++  ***
#******************
download http://ftp.gnome.org/pub/GNOME/sources/libxml++/2.26/libxml++-2.26.1.tar.gz
tar xzf libxml++-2.26.1.tar.gz
cd libxml++-2.26.1
	INSTALL_DIR=libxml++-2.26.1
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

#*********************
#***  ImageMagick  ***
#*********************
download ftp://ftp.imagemagick.org/pub/ImageMagick/ImageMagick-6.5.8-2.tar.gz
tar xzf ImageMagick-6.5.8-2.tar.gz
cd ImageMagick-6.5.8-2
	INSTALL_DIR=ImageMagick-6.5.8-2
	
	LDFLAGS="-L/build/zlib-1.2.3/lib $LDFLAGS" ./configure --prefix=$BUILD_DIR/$INSTALL_DIR --disable-dependency-tracking --disable-installed --enable-delegate-build --disable-shared --enable-static --enable-libtool-verbose --without-rsvg --without-gslib --without-gvc --without-wmf --without-frozenpaths --without-x --disable-openmp && make && make install
	add_paths
cd ..

#********************
#***     GLEW     ***
#********************
download http://surfnet.dl.sourceforge.net/project/glew/glew/1.5.1/glew-1.5.1-src.tgz
tar xzf glew-1.5.1-src.tgz
cd glew
	INSTALL_DIR=glew-1.5.1
	
	make
	
	mkdir -p $BUILD/$INSTALL_DIR/bin
	mkdir -p $BUILD/$INSTALL_DIR/lib
	mkdir -p $BUILD/$INSTALL_DIR/include
	cp bin/*.exe /mingw/bin
	cp lib/glew32.dll $BUILD/$INSTALL_DIR/bin/glew32.dll
	cp lib/libglew32.a $BUILD/$INSTALL_DIR/lib/libglew32.a
	cp lib/libglew32.dll.a $BUILD/$INSTALL_DIR/lib/libglew32.dll.a
	cp include/* $BUILD/$INSTALL_DIR/include/
	add_paths
cd ..


#********************
#***    FFMPEG    ***
#********************
download http://ffmpeg.org/releases/ffmpeg-0.5.tar.bz2
tar xjf ffmpeg-0.5.tar.bz2
cd ffmpeg-0.5
	INSTALL_DIR=ffmpeg-0.5
	
	#to check this line, maybe some options can be reactivated
	./configure --prefix=$BUILD/$INSTALL_DIR --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-memalign-hack --enable-static --enable-shared --enable-gpl --disable-encoders --disable-muxers --disable-bsfs --disable-ffmpeg --disable-ffserver --disable-ffplay --disable-protocol=http --disable-protocol=rtp --disable-protocol=tcp --disable-protocol=udp && make && make install
	add_paths
cd ..

#*********************
#***   Portaudio   ***
#*********************
download http://www.portaudio.com/archives/pa_stable_v19_20071207.tar.gz
tar xzf pa_stable_v19_20071207.tar.gz
cd portaudio
	INSTALL_DIR=portaudio
	
	./configure --prefix=$BUILD_DIR/$INSTALL_DIR && make && make install
	add_paths
cd ..

echo "export CPATH=$CPATH" > paths-to-add.sh
echo "export PATH=$PATH" >> paths-to-add.sh
echo "export PKG_CONFIG_PATH=$PKG_CONFIG_PATH" >> paths-to-add.sh
echo "export LIBRARY_PATH=$LIBRARY_PATH" >> paths-to-add.sh
echo
echo "Enviroment variables exported to paths-to-add.sh. Run this file to get the changes of you Env variables"
echo
export CPATH
export PATH
export LIBRARY_PATH
export PKG_CONFIG_PATH