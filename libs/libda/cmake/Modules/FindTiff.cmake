# - Try to find Tiff
# Once done, this will define
#
#  Tiff_FOUND - system has Tiff
#  Tiff_INCLUDE_DIRS - the Tiff include directories
#  Tiff_LIBRARIES - link these to use Tiff
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Tiff_PKGCONF tiff)

find_path(Tiff_INCLUDE_DIR
  NAMES tiffconf.h
  PATHS ${Tiff_PKGCONF_INCLUDE_DIRS}
)

find_library(Tiff_LIBRARY
  NAMES libtiff
  PATHS ${Tiff_PKGCONF_LIBRARY_DIRS}
)

set(Tiff_PROCESS_INCLUDES Tiff_INCLUDE_DIR)
set(Tiff_PROCESS_LIBS Tiff_LIBRARY)
libfind_process(Tiff)

