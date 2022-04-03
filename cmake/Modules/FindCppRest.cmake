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

if(WIN32)
	libfind_package(CppRest OpenSSL)
else()
	libfind_package(CppRest Crypto)
	libfind_package(CppRest Ssl)
endif()

libfind_pkg_detect(CppRest cpprest FIND_PATH http_client.h PATH_SUFFIXES cpprest FIND_LIBRARY cpprest cpprest_2_10d)
set(CppRest_VERSION ${CppRest_PKGCONF_VERSION})
libfind_process(CppRest)