# - Try to find FFMPEG libavutil
# Once done, this will define
#
#  AVUtil_FOUND - the library is available
#  AVUtil_INCLUDE_DIRS - the include directories
#  AVUtil_LIBRARIES - the libraries
#  AVUtil_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(AVUtil_PKGCONF libavutil)

find_path(AVUtil_INCLUDE_DIR
  NAMES libavutil/avutil.h ffmpeg/avutil.h avutil.h
  HINTS ${AVUtil_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)

if(AVUtil_INCLUDE_DIR)
  foreach(suffix libavutil/ ffmpeg/ "")
    if(NOT AVUtil_INCLUDE)
      if(EXISTS "${AVUtil_INCLUDE_DIR}/${suffix}avutil.h")
        set(AVUtil_INCLUDE "${suffix}avutil.h")
      endif(EXISTS "${AVUtil_INCLUDE_DIR}/${suffix}avutil.h")
    endif(NOT AVUtil_INCLUDE)
  endforeach(suffix)

  if(NOT AVUtil_INCLUDE)
    message(FATAL_ERROR "Found avutil.h include dir, but not the header file. Perhaps you need to clear CMake cache?")
  endif(NOT AVUtil_INCLUDE)
endif(AVUtil_INCLUDE_DIR)

find_library(AVUtil_LIBRARY
  NAMES libavutil.dll.a avutil
  HINTS ${AVUtil_PKGCONF_LIBRARY_DIRS}
)

libfind_process(AVUtil)

