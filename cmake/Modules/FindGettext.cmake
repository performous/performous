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

libfind_pkg_check_modules(Gettext_PKGCONF Gettext)

find_path(Gettext_INCLUDE_DIR
  NAMES libintl.h
  PATHS ${Gettext_PKGCONF_INCLUDE_DIRS}
)

find_library(Gettext_LIBRARY
  NAMES libintl
  PATHS ${Gettext_PKGCONF_LIBRARY_DIRS}
)

set(Gettext_PROCESS_INCLUDES Gettext_INCLUDE_DIR)
set(Gettext_PROCESS_LIBS Gettext_LIBRARY)
libfind_process(gettext)
