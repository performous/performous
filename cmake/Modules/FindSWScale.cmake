# - Try to find FFMPEG libswscale
# Once done, this will define
#
#  SWScale_FOUND - the library is available
#  SWScale_INCLUDE_DIRS - the include directories
#  SWScale_LIBRARIES - the libraries
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(SWScale AVUtil)

libfind_pkg_check_modules(SWScale_PKGCONF libswscale)

find_path(SWScale_INCLUDE_DIR
  NAMES swscale.h
  PATHS ${SWScale_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg ffmpeg/swscale swscale libswscale
)

find_library(SWScale_LIBRARY
  NAMES swscale
  PATHS ${SWScale_PKGCONF_LIBRARY_DIRS}
)

set(SWScale_PROCESS_INCLUDES SWScale_INCLUDE_DIR AVUtil_INCLUDE_DIRS)
set(SWScale_PROCESS_LIBS SWScale_LIBRARY AVUtil_LIBRARIES)
libfind_process(SWScale)

