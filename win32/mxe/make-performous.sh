#!/bin/bash -e

MXE_PREFIX=/opt/mxe
JOBS=`nproc 2>/dev/null`
STAGE="`pwd`/stage"
BUILD_TYPE="Release"
if [ "$1" == "debug" ]; then
	BUILD_TYPE="Debug"
	echo "Building with debug symbols"
	shift
fi

mkdir -p build
cd build

cmake ../../.. \
	-DMXE_HACK=ON \
	-DPKG_CONFIG_EXECUTABLE="$MXE_PREFIX/usr/bin/i686-pc-mingw32-pkg-config" \
	-DCMAKE_TOOLCHAIN_FILE="$MXE_PREFIX/usr/i686-pc-mingw32/share/cmake/mxe-conf.cmake" \
	-DBoost_thread_LIBRARY="$MXE_PREFIX/usr/i686-pc-mingw32/lib/libboost_thread_win32-mt.a" \
	-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
	-DCMAKE_INSTALL_PREFIX="$STAGE" \
	-DNO_WEBCAM=ON \
	-DENABLE_TOOLS=OFF

if [ "$1" != "config" ]; then
	make -j $JOBS
	make install
	python ../copydlls.py "$MXE_PREFIX/usr/i686-pc-mingw32/bin" "$STAGE/bin"
fi

echo "Done"

