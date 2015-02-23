#!/bin/sh -e
here="$(pwd)"
MXE_PREFIX="$here/mxe"
MXE_TARGET=i686-w64-mingw32.shared
mkdir -p build
mkdir -p stage
cd build
cmake ../../.. -DPKG_CONFIG_EXECUTABLE="$MXE_PREFIX/usr/bin/$MXE_TARGET-pkg-config" \
  -DCMAKE_TOOLCHAIN_FILE="$MXE_PREFIX/usr/$MXE_TARGET/share/cmake/mxe-conf.cmake" \
  -DBoost_THREAD_LIBRARY_RELEASE="$MXE_PREFIX/usr/$MXE_TARGET/bin/libboost_thread_win32-mt.dll" \
  -DENABLE_WEBCAM=OFF -DCMAKE_INSTALL_PREFIX="$here/stage"
make install
