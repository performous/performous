# - Try to find Gettext
# Once done, this will define
#
#  Gettext_FOUND - system has Gettext
#  Gettext_INCLUDE_DIRS - the Gettext include directories
#  Gettext_LIBRARIES - link these to use Gettext
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(gettext_PKGCONF gettext)

find_path(gettext_INCLUDE_DIR
  NAMES libintl.h
  PATHS ${gettext_PKGCONF_INCLUDE_DIRS}
)

find_library(gettext_LIBRARY
  NAMES libintl
  PATHS ${gettext_PKGCONF_LIBRARY_DIRS}
)

set(gettext_PROCESS_INCLUDES gettext_INCLUDE_DIR)
set(gettext_PROCESS_LIBS gettext_LIBRARY)
libfind_process(Gettext)
