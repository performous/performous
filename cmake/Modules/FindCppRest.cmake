# - Try to find cpprest
# Once done, this will define
#
#  CppRest_FOUND - system has CppRest
#  CppRest_INCLUDE_DIRS - the CppRest include directories
#  CppRest_LIBRARIES - link these to use CppRest
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(CppRest OpenSSL)

libfind_pkg_detect(CppRest cpprest FIND_PATH http_client.h PATH_SUFFIXES cpprest FIND_LIBRARY cpprest)
set(CppRest_VERSION ${CppRest_PKGCONF_VERSION})
libfind_process(CppRest)