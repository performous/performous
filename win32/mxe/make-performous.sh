#!/bin/bash -e
mkdir build
mkdir install-folder
cd build
MXE_PREFIX=../mxe
 MXE_TARGET=i686-w64-mingw32.shared
 cmake ../../.. -DPKG_CONFIG_EXECUTABLE=$MXE_PREFIX/usr/bin/$MXE_TARGET-pkg-config \
  -DCMAKE_TOOLCHAIN_FILE=$MXE_PREFIX/usr/$MXE_TARGET/share/cmake/mxe-conf.cmake \
  -DBoost_THREAD_LIBRARY_RELEASE=$MXE_PREFIX/usr/$MXE_TARGET/lib/libboost_thread_win32-mt.dll \
  -DENABLE_WEBCAM=OFF -DCMAKE_INSTALL_PREFIX=../install-folder
make install

