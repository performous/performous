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
  HINTS ${Gettext_PKGCONF_INCLUDE_DIRS}
)
set(Gettext_PROCESS_INCLUDES Gettext_INCLUDE_DIR)

# On "traditional" UNIX systems (Solaris, BSD, Mac OS X, etc.),
# libintl is its own library. On GNU/glibc systems (Linux), gettext
# functionality is integrated into glibc, so we always have working
# internationalization support. Therefore we try to find a libintl
# but ignore the fact that it may not exist.

find_library(Gettext_LIBRARY
  NAMES intl
  HINTS ${Gettext_PKGCONF_LIBRARY_DIRS}
  PATHS /opt/local/lib /sw/local/lib
)

if (Gettext_LIBRARY)
  set(Gettext_PROCESS_LIBS Gettext_LIBRARY)
else()
  unset(Gettext_LIBRARY CACHE)
endif()

libfind_process(Gettext)
