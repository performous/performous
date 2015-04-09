# - Try to find libav libavresample
# Once done, this will define
#
#  AVResample_FOUND - the library is available
#  AVResample_INCLUDE_DIRS - the include directories
#  AVResample_LIBRARIES - the libraries
#  AVResample_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVResample AVUtil)



libfind_pkg_check_modules(AVResample_PKGCONF libavresample)

find_path(AVResample_INCLUDE_DIR
  NAMES libavresample/avresample.h ffmpeg/avresample.h avresample.h
  HINTS ${AVResample_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)
if(AVResample_INCLUDE_DIR)
  foreach(suffix libavresample/ ffmpeg/ "")
    if(NOT AVResample_INCLUDE)
      if(EXISTS "${AVResample_INCLUDE_DIR}/${suffix}avresample.h")
        set(AVResample_INCLUDE "${suffix}avresample.h")
      endif(EXISTS "${AVResample_INCLUDE_DIR}/${suffix}avresample.h")
    endif(NOT AVResample_INCLUDE)
  endforeach(suffix)

    if(NOT AVResample_INCLUDE)
    message(FATAL_ERROR "Found avresample.h include dir, but not the header file. Perhaps you need to clear CMake cache?")
  endif(NOT AVResample_INCLUDE)
endif(AVResample_INCLUDE_DIR)

find_library(AVResample_LIBRARY
  NAMES libavresample.dll.a avresample
  HINTS ${AVResample_PKGCONF_LIBRARY_DIRS}
)

libfind_process(AVResample)
