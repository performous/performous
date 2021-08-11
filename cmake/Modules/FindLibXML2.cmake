# - Try to find LibXML2
# Once done, this will define
#
#  LibXML2_FOUND - system has LibXML2
#  LibXML2_INCLUDE_DIRS - the LibXML2 include directories
#  LibXML2_LIBRARIES - link these to use LibXML2
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(LibXML2_PKGCONF libxml-2.0)

find_path(LibXML2_INCLUDE_DIR
  NAMES libxml/xpath.h
  HINTS ${LibXML2_PKGCONF_INCLUDE_DIRS}
  HINTS ${LibXML2_PKGCONF_INCLUDE_DIRS}/libxml2
  PATH_SUFFIXES libxml2
)

find_library(LibXML2_LIBRARY
  NAMES xml2 libxml2
  HINTS ${LibXML2_PKGCONF_LIBRARY_DIRS}
)

set(LibXML2_PROCESS_INCLUDES LibXML2_INCLUDE_DIR)
set(LibXML2_PROCESS_LIBS LibXML2_LIBRARY)
libfind_process(LibXML2)

