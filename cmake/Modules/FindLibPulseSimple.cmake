# - Try to find LibPulseSimple
# Once done, this will define
#
#  LibPulseSimple_FOUND - system has LibPulseSimple
#  LibPulseSimple_INCLUDE_DIRS - the LibPulseSimple include directories
#  LibPulseSimple_LIBRARIES - link these to use LibPulseSimple
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(LibPulseSimple_PKGCONF libpulse-simple)

find_path(LibPulseSimple_INCLUDE_DIR
  NAMES pulse/simple.h
  HINTS ${LibPulseSimple_PKGCONF_INCLUDE_DIRS}
)

find_library(LibPulseSimple_LIBRARY
  NAMES pulse-simple
  HINTS ${LibPulseSimple_PKGCONF_LIBRARY_DIRS}
)

set(LibPulseSimple_PROCESS_INCLUDES LibPulseSimple_INCLUDE_DIR)
set(LibPulseSimple_PROCESS_LIBS LibPulseSimple_LIBRARY)
libfind_process(LibPulseSimple)

