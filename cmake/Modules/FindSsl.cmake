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

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Ssl_PKGCONF ssl)

# Include dir
find_path(Ssl_INCLUDE_DIR
  NAMES openssl/ssl.h
  PATHS ${Ssl_PKGCONF_INCLUDE_DIRS}# /usr/include/boost/
)

# Finally the library itself
find_library(Ssl_LIBRARY
  NAMES ssl
  PATHS ${Ssl_PKGCONF_LIBRARY_DIRS}
)

###TODO Extract the version number
#if(Ssl_INCLUDE_DIR)
  # Extract the version number
#  file(READ "${Ssl_INCLUDE_DIR}/alsa/version.h" _ALSA_VERSION_H_CONTENTS)
#  string(REGEX REPLACE ".*#define SND_LIB_VERSION_STR[ \t]*\"([^\n]*)\".*" "\\1" ALSA_VERSION "${_ALSA_VERSION_H_CONTENTS}")
#endif(Ssl_INCLUDE_DIR)

set(Ssl_PROCESS_INCLUDES Ssl_INCLUDE_DIR)
set(Ssl_PROCESS_LIBS Ssl_LIBRARY)
libfind_process(Ssl)
