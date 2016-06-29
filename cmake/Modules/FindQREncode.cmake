# Locates the qrencode library
# This module defines
# QRENCODE_LIBRARY, the name of the library to link against
# QRENCODE_FOUND, if false, do not try to link to SDL
# QRENCODE_INCLUDE_DIR, where to find SDL.h
#
# QRENCODE_DIR: specify optional search dir

include(LibFindMacros)
find_path(QREncode_INCLUDE_DIR NAMES qrencode.h)
find_library(QREncode_LIBRARY NAMES qrencode)
set(QREncode_PROCESS_INCLUDES QREncode_INCLUDE_DIR)
set(QREncode_PROCESS_LIBS QREncode_LIBRARY)

libfind_process(QREncode)


