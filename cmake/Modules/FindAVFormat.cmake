# - Try to find FFMPEG libavformat
# Once done, this will define
#
#  AVFormat_FOUND - the library is available
#  AVFormat_INCLUDE_DIRS - the include directories
#  AVFormat_LIBRARIES - the libraries
#  AVFormat_INCLUDE - the file to include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVFormat AVCodec)

libfind_pkg_check_modules(AVFormat_PKGCONF libavformat)

find_path(AVFormat_INCLUDE_DIR
  NAMES libavformat/avformat.h ffmpeg/avformat.h avformat.h
  HINTS ${AVFormat_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)

if(AVFormat_INCLUDE_DIR)
  foreach(suffix libavformat/ ffmpeg/ "")
    if(NOT AVFormat_INCLUDE)
      if(EXISTS "${AVFormat_INCLUDE_DIR}/${suffix}avformat.h")
        set(AVFormat_INCLUDE "${suffix}avformat.h")
      endif(EXISTS "${AVFormat_INCLUDE_DIR}/${suffix}avformat.h")
    endif(NOT AVFormat_INCLUDE)
  endforeach(suffix)

  if(NOT AVFormat_INCLUDE)
    message(FATAL_ERROR "Found avformat.h include dir, but not the header file. Perhaps you need to clear CMake cache?")
  endif(NOT AVFormat_INCLUDE)
endif(AVFormat_INCLUDE_DIR)

find_library(AVFormat_LIBRARY
  NAMES libavformat.dll.a avformat
  HINTS ${AVFormat_PKGCONF_LIBRARY_DIRS}
)

libfind_process(AVFormat)

