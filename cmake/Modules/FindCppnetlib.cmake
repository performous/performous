# - Try to find Cppnetlib
# Once done, this will define
#
#  Cppnetlib_FOUND - system has Cppnetlib
#  Cppnetlib_INCLUDE_DIRS - the Cppnetlib include directories
#  Cppnetlib_LIBRARIES - link these to use Cppnetlib
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

#Use find_package to detect other libraries that the library depends on
libfind_package(Cppnetlib Boost)
libfind_package(Cppnetlib Crypto)
libfind_package(Cppnetlib Ssl)

# Include dir
find_path(Cppnetlib_INCLUDE_DIR NAMES boost/network.hpp)

# Library components
find_library(Cppnetlib_uri_LIBRARY NAMES cppnetlib-uri)
find_library(Cppnetlib_client_connections_LIBRARY NAMES cppnetlib-client-connections)
find_library(Cppnetlib_server_parsers_LIBRARY NAMES cppnetlib-server-parsers)

set(Cppnetlib_PROCESS_INCLUDES Cppnetlib_INCLUDE_DIR BOOST_INCLUDE_DIRS Crypto_INCLUDE_DIRS Ssl_INCLUDE_DIRS)
set(Cppnetlib_PROCESS_LIBS Cppnetlib_uri_LIBRARY Cppnetlib_client_connections_LIBRARY Cppnetlib_server_parsers_LIBRARY BOOST_LIBRARIES Crypto_LIBRARIES Ssl_LIBRARIES)
libfind_process(Cppnetlib)
