#!/bin/bash -e

MXE_PREFIX=/opt/mxe

mkdir -p build
cd build

cmake ../../.. \
	-DCMAKE_TOOLCHAIN_FILE=$MXE_PREFIX/usr/i686-pc-mingw32/share/cmake/mxe-conf.cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="`pwd`/stage" \
	-DNO_WEBCAM=YES \
	-DENABLE_TOOLS=OFF

make
make install

