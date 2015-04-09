# - Try to find FFMPEG libavcodec
# Once done, this will define
#
#  AVCodec_FOUND - the library is available
#  AVCodec_INCLUDE_DIRS - the include directories
#  AVCodec_LIBRARIES - the libraries
#  AVCodec_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVCodec AVUtil)

# TODO: pkg-config extra deps: libraw1394 theora vorbisenc

libfind_pkg_check_modules(AVCodec_PKGCONF libavcodec)

find_path(AVCodec_INCLUDE_DIR
  NAMES libavcodec/avcodec.h ffmpeg/avcodec.h avcodec.h
  HINTS ${AVCodec_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)

if(AVCodec_INCLUDE_DIR)
  foreach(suffix libavcodec/ ffmpeg/ "")
    if(NOT AVCodec_INCLUDE)
      if(EXISTS "${AVCodec_INCLUDE_DIR}/${suffix}avcodec.h")
        set(AVCodec_INCLUDE "${suffix}avcodec.h" CACHE INTERNAL "")
      endif(EXISTS "${AVCodec_INCLUDE_DIR}/${suffix}avcodec.h")
    endif(NOT AVCodec_INCLUDE)
  endforeach(suffix)

  if(NOT AVCodec_INCLUDE)
    message(FATAL_ERROR "Found avcodec.h include dir, but not the header file. Perhaps you need to clear CMake cache?")
  endif(NOT AVCodec_INCLUDE)
endif(AVCodec_INCLUDE_DIR)

find_library(AVCodec_LIBRARY
  NAMES libavcodec.dll.a avcodec
  HINTS ${AVCodec_PKGCONF_LIBRARY_DIRS}
)

libfind_process(AVCodec)

