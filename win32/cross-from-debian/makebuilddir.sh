#!/bin/sh -e
mkdir -pv build
cd build
exec env PATH="`pwd`/../deps/bin:$PATH" cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake -DCMAKE_INSTALL_PREFIX="`pwd`/../stage" "$@" ../../..
