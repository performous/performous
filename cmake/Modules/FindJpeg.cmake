# - Try to find Jpeg
# Once done, this will define
#
#  Jpeg_FOUND - system has Jpeg
#  Jpeg_INCLUDE_DIRS - the Jpeg include directories
#  Jpeg_LIBRARIES - link these to use Jpeg
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Jpeg_PKGCONF jpeg)

find_path(Jpeg_INCLUDE_DIR
  NAMES jconfig.h jpeglib.h
  PATHS ${Jpeg_PKGCONF_INCLUDE_DIRS}
)

find_library(Jpeg_LIBRARY
  NAMES jpeg
  PATHS ${Jpeg_PKGCONF_LIBRARY_DIRS}
)

set(Jpeg_PROCESS_INCLUDES Jpeg_INCLUDE_DIR)
set(Jpeg_PROCESS_LIBS Jpeg_LIBRARY)
libfind_process(Jpeg)

