# - Try to find FFMPEG libavcodec
# Once done, this will define
#
#  AVCodec_FOUND - the library is available
#  AVCodec_INCLUDE_DIRS - the include directories
#  AVCodec_LIBRARIES - the libraries
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVCodec AVUtil)

# TODO: pkg-config extra deps: libraw1394 theora vorbisenc

libfind_pkg_check_modules(AVCodec_PKGCONF libavcodec)

find_path(AVCodec_INCLUDE_DIR
  NAMES avcodec.h
  PATHS ${AVCodec_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg ffmpeg/avcodec avcodec libavcodec ffmpeg/libavcodec
)

find_library(AVCodec_LIBRARY
  NAMES avcodec
  PATHS ${AVCodec_PKGCONF_LIBRARY_DIRS}
)

set(AVCodec_PROCESS_INCLUDES AVCodec_INCLUDE_DIR AVUtil_INCLUDE_DIRS)
set(AVCodec_PROCESS_LIBS AVCodec_LIBRARY AVUtil_LIBRARIES)
libfind_process(AVCodec)

