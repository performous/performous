# - Try to find Jack
# Once done, this will define
#
#  Jack_FOUND - system has Jack
#  Jack_INCLUDE_DIRS - the Jack include directories
#  Jack_LIBRARIES - link these to use Jack
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Jack_PKGCONF jack)

find_path(Jack_INCLUDE_DIR
  NAMES jack/jack.h
  HINTS ${Jack_PKGCONF_INCLUDE_DIRS}
)

find_library(Jack_LIBRARY
  NAMES jack
  HINTS ${Jack_PKGCONF_LIBRARY_DIRS}
)

set(Jack_PROCESS_INCLUDES Jack_INCLUDE_DIR)
set(Jack_PROCESS_LIBS Jack_LIBRARY)
libfind_process(Jack)

