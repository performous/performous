# - Try to find LibPulse
# Once done, this will define
#
#  LibPulse_FOUND - system has LibPulse
#  LibPulse_INCLUDE_DIRS - the LibPulse include directories
#  LibPulse_LIBRARIES - link these to use LibPulse
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(LibPulse_PKGCONF libpulse)

find_path(LibPulse_INCLUDE_DIR
  NAMES pulse/pulseaudio.h
  HINTS ${LibPulse_PKGCONF_INCLUDE_DIRS}
)

find_library(LibPulse_LIBRARY
  NAMES pulse
  HINTS ${LibPulse_PKGCONF_LIBRARY_DIRS}
)

set(LibPulse_PROCESS_INCLUDES LibPulse_INCLUDE_DIR)
set(LibPulse_PROCESS_LIBS LibPulse_LIBRARY)
libfind_process(LibPulse)

