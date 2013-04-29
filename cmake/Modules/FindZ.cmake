# - Try to find Z
# Once done, this will define
#
#  Z_FOUND - system has Z
#  Z_INCLUDE_DIRS - the Z include directories
#  Z_LIBRARIES - link these to use Z
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Z_PKGCONF Z)

find_path(Z_INCLUDE_DIR
  NAMES zlib.h
  PATHS ${Z_PKGCONF_INCLUDE_DIRS}
)

find_library(Z_LIBRARY
  NAMES z
  PATHS ${Z_PKGCONF_LIBRARY_DIRS}
)

set(Z_PROCESS_INCLUDES Z_INCLUDE_DIR)
set(Z_PROCESS_LIBS Z_LIBRARY)
libfind_process(Z)

