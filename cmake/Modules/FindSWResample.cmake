# - Try to find FFMPEG libswresample
# Once done, this will define
#
#  SWResample_FOUND - the library is available
#  SWResample_INCLUDE_DIRS - the include directories
#  SWResample_LIBRARIES - the libraries
#  SWResample_INCLUDE - the file to include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(SWResample AVUtil)

libfind_pkg_check_modules(SWResample_PKGCONF libswresample)

find_path(SWResample_INCLUDE_DIR
  NAMES libswresample/swresample.h ffmpeg/swresample.h swresample.h
  HINTS ${SWResample_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)

if(SWResample_INCLUDE_DIR)
  foreach(suffix libswresample/ ffmpeg/ "")
    if(NOT SWResample_INCLUDE)
      if(EXISTS "${SWResample_INCLUDE_DIR}/${suffix}swresample.h")
        set(SWResample_INCLUDE "${suffix}swresample.h")
      endif(EXISTS "${SWResample_INCLUDE_DIR}/${suffix}swresample.h")
    endif(NOT SWResample_INCLUDE)
  endforeach(suffix)

  if(NOT SWResample_INCLUDE)
    message(FATAL_ERROR "Found swresample.h include dir, but not the header file. Maybe you need to clear CMake cache?")
  endif(NOT SWResample_INCLUDE)
endif(SWResample_INCLUDE_DIR)

find_library(SWResample_LIBRARY
  NAMES libswresample.dll.a swresample
  HINTS ${SWResample_PKGCONF_LIBRARY_DIRS}
)

libfind_process(SWResample)

