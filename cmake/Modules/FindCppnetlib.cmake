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
find_package(Crypto REQUIRED)
find_package(Ssl REQUIRED)
#libfind_package(Cppnetlib Crypto)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Cppnetlib_PKGCONF cpp-netlib)

# Include dir
find_path(Cppnetlib_INCLUDE_DIR
  NAMES network.hpp
  #PATHS ${Cppnetlib_PKGCONF_INCLUDE_DIRS}# /usr/include/boost/
  PATHS /usr/include/boost/ /usr/local/include/boost/
)

# Finally the library itself
find_library(Cppnetlib_LIBRARY
  NAMES  libcppnetlib-uri.a libcppnetlib-client-connections.a libcppnetlib-server-parsers.a
  #PATHS ${Cppnetlib_PKGCONF_LIBRARY_DIRS}
  PATHS /usr/lib/ /usr/loca/lib/
)

#if(Cppnetlib_INCLUDE_DIR)
  # Extract the version number
#  file(READ "${Cppnetlib_INCLUDE_DIR}/alsa/version.h" _ALSA_VERSION_H_CONTENTS)
#  string(REGEX REPLACE ".*#define SND_LIB_VERSION_STR[ \t]*\"([^\n]*)\".*" "\\1" ALSA_VERSION "${_ALSA_VERSION_H_CONTENTS}")
#endif(Cppnetlib_INCLUDE_DIR)

set(Cppnetlib_PROCESS_INCLUDES Cppnetlib_INCLUDE_DIR) #BOOST_INCLUDE_DIRS
set(Cppnetlib_PROCESS_LIBS Cppnetlib_LIBRARY) #BOOST_LIBRARIES
libfind_process(Cppnetlib)
