# - Try to find Ssl
# Once done, this will define
#
#  Ssl_FOUND - system has Ssl
#  Ssl_INCLUDE_DIRS - the Ssl include directories
#  Ssl_LIBRARIES - link these to use Ssl
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)
libfind_pkg_detect(Ssl libssl FIND_PATH openssl/ssl.h FIND_LIBRARY ssl)
set(Ssl_VERSION ${Ssl_PKGCONF_VERSION})
libfind_process(Ssl)