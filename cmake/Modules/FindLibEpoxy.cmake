# - Try to find LibEpoxy
# Once done, this will define
#
#  LibEpoxy_FOUND - system has LibEpoxy
#  LibEpoxy_INCLUDE_DIRS - the LibEpoxy include directories
#  LibEpoxy_LIBRARIES - link these to use LibEpoxy
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(LibEpoxy_PKGCONF epoxy)

find_path(LibEpoxy_INCLUDE_DIR
  NAMES epoxy/gl.h
  PATHS ${LibEpoxy_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES LibEpoxy
)

find_library(LibEpoxy_LIBRARY
  NAMES epoxy
  PATHS ${LibEpoxy_PKGCONF_LIBRARY_DIRS}
)

set(LibEpoxy_PROCESS_INCLUDES LibEpoxy_INCLUDE_DIR)
set(LibEpoxy_PROCESS_LIBS LibEpoxy_LIBRARY)
libfind_process(LibEpoxy)

