# - Try to find Png
# Once done, this will define
#
#  Png_FOUND - system has Png
#  Png_INCLUDE_DIRS - the Png include directories
#  Png_LIBRARIES - link these to use Png
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Png_PKGCONF Png)

find_path(Png_INCLUDE_DIR
  NAMES pngconf.h
  PATHS ${Png_PKGCONF_INCLUDE_DIRS}
)

find_library(Png_LIBRARY
  NAMES png12
  PATHS ${Png_PKGCONF_LIBRARY_DIRS}
)

set(Png_PROCESS_INCLUDES Png_INCLUDE_DIR)
set(Png_PROCESS_LIBS Png_LIBRARY)
libfind_process(Png)

