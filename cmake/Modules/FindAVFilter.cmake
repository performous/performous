# - Try to find FFMPEG libavfilter
# Once done, this will define
#
#  AVFilter_FOUND - the library is available
#  AVFilter_INCLUDE_DIRS - the include directories
#  AVFilter_LIBRARIES - the libraries
#  AVFilter_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVFilter AVUtil)

# TODO: pkg-config extra deps: libraw1394 theora vorbisenc

libfind_pkg_check_modules(AVFilter_PKGCONF libavfilter)

find_path(AVFilter_INCLUDE_DIR
  NAMES libavfilter/avfilter.h ffmpeg/avfilter.h avfilter.h
  HINTS ${AVFilter_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg
)

if(AVFilter_INCLUDE_DIR)
  foreach(suffix libavfilter/ ffmpeg/ "")
    if(NOT AVFilter_INCLUDE)
      if(EXISTS "${AVFilter_INCLUDE_DIR}/${suffix}avfilter.h")
        set(AVFilter_INCLUDE "${suffix}avfilter.h" CACHE INTERNAL "")
      endif(EXISTS "${AVFilter_INCLUDE_DIR}/${suffix}avfilter.h")
    endif(NOT AVFilter_INCLUDE)
  endforeach(suffix)

  if(NOT AVFilter_INCLUDE)
    message(FATAL_ERROR "Found avfilter.h include dir, but not the header file. Perhaps you need to clear CMake cache?")
  endif(NOT AVFilter_INCLUDE)
endif(AVFilter_INCLUDE_DIR)

find_library(AVFilter_LIBRARY
  NAMES libavfilter.dll.a avfilter
  HINTS ${AVFilter_PKGCONF_LIBRARY_DIRS}
)

libfind_process(AVFilter)

