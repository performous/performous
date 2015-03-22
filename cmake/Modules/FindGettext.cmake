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
INCLUDE(CheckSymbolExists)

libfind_pkg_check_modules(Gettext_PKGCONF Gettext)

find_path(Gettext_INCLUDE_DIR
  NAMES libintl.h
  HINTS ${Gettext_PKGCONF_INCLUDE_DIRS}
)
set(Gettext_PROCESS_INCLUDES Gettext_INCLUDE_DIR)

# On "traditional" UNIX systems (Solaris, BSD, Mac OS X, etc.),
# libintl is its own library. On GNU/glibc systems (Linux), gettext
# functionality is integrated into glibc, so we always have working
# internationalization support.

CHECK_SYMBOL_EXISTS(bindtextdomain "libintl.h" Gettext_HAS_BINDTEXTDOMAIN_WITHOUT_LIB)

if(NOT Gettext_HAS_BINDTEXTDOMAIN_WITHOUT_LIB)
  find_library(Gettext_LIBRARY
	NAMES intl
	HINTS ${Gettext_PKGCONF_LIBRARY_DIRS}
	PATHS /opt/local/lib /sw/local/lib
  )

  set(Gettext_PROCESS_LIBS Gettext_LIBRARY)
endif()

libfind_process(Gettext)
