# - Try to find Fontconfig
# Once done, this will define
#
#  Fontconfig_FOUND - system has Fontconfig
#  Fontconfig_INCLUDE_DIRS - the Fontconfig include directories
#  Fontconfig_LIBRARIES - link these to use Fontconfig
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Fontconfig_PKGCONF Fontconfig)

find_path(Fontconfig_INCLUDE_DIR
  NAMES fontconfig/fontconfig.h
  HINTS ${Fontconfig_PKGCONF_INCLUDE_DIRS}
)

find_library(Fontconfig_LIBRARY
  NAMES fontconfig
  HINTS ${Fontconfig_PKGCONF_LIBRARY_DIRS}
)

set(Fontconfig_PROCESS_INCLUDES Fontconfig_INCLUDE_DIR)
set(Fontconfig_PROCESS_LIBS Fontconfig_LIBRARY)
libfind_process(Fontconfig)

