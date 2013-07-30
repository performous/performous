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

###TODO cpp-netlib depends on boost-asio. check it first

#find_package( Cppnetlib REQUIRED ${Cppnetlib_COMPONENTS})

#Use find_package to detect other libraries that the library depends on
libfind_package(Cppnetlib Boost)
libfind_package(Cppnetlib Crypto)
libfind_package(Cppnetlib Ssl)

# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(Cppnetlib_PKGCONF cpp-netlib)

# Include dir
find_path(Cppnetlib_INCLUDE_DIR
  NAMES network.hpp
  #PATHS ${Cppnetlib_PKGCONF_INCLUDE_DIRS}# /usr/include/boost/
  PATHS /usr/include/boost/ /usr/local/include/boost/
)

# Finally the library components

find_library(Cppnetlib_uri_LIBRARY
  NAMES  libcppnetlib-uri.a
  #PATHS ${Cppnetlib_PKGCONF_LIBRARY_DIRS}
)

find_library(Cppnetlib_client_connections_LIBRARY
  NAMES  libcppnetlib-client-connections.a
  #PATHS ${Cppnetlib_PKGCONF_LIBRARY_DIRS}
)

find_library(Cppnetlib_server_parsers_LIBRARY
  NAMES  libcppnetlib-server-parsers.a
  #PATHS ${Cppnetlib_PKGCONF_LIBRARY_DIRS}
)

#if(Cppnetlib_INCLUDE_DIR)
  # Extract the version number
#  file(READ "${Cppnetlib_INCLUDE_DIR}/alsa/version.h" _ALSA_VERSION_H_CONTENTS)
#  string(REGEX REPLACE ".*#define SND_LIB_VERSION_STR[ \t]*\"([^\n]*)\".*" "\\1" ALSA_VERSION "${_ALSA_VERSION_H_CONTENTS}")
#endif(Cppnetlib_INCLUDE_DIR)

set(Cppnetlib_PROCESS_INCLUDES Cppnetlib_INCLUDE_DIR BOOST_INCLUDE_DIRS Crypto_INCLUDE_DIRS Ssl_INCLUDE_DIRS)
set(Cppnetlib_PROCESS_LIBS Cppnetlib_uri_LIBRARY Cppnetlib_client_connections_LIBRARY Cppnetlib_server_parsers_LIBRARY BOOST_LIBRARIES Crypto_LIBRARIES Ssl_LIBRARIES)
libfind_process(Cppnetlib)
