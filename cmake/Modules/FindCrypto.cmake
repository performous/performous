# - Try to find Crypto
# Once done, this will define
#
#  Crypto_FOUND - system has Crypto
#  Crypto_INCLUDE_DIRS - the Crypto include directories
#  Crypto_LIBRARIES - link these to use Crypto
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Crypto_PKGCONF libcrypto)

# Include dir
find_path(Crypto_INCLUDE_DIR
  NAMES openssl/crypto.h
  PATHS ${Crypto_PKGCONF_INCLUDE_DIRS}# /usr/include/boost/
)

# Finally the library itself
find_library(Crypto_LIBRARY
  NAMES crypto
  PATHS ${Crypto_PKGCONF_LIBRARY_DIRS}
)

###TODO Extract the version number
#if(Crypto_INCLUDE_DIR)
  # Extract the version number
#  file(READ "${Crypto_INCLUDE_DIR}/alsa/version.h" _ALSA_VERSION_H_CONTENTS)
#  string(REGEX REPLACE ".*#define SND_LIB_VERSION_STR[ \t]*\"([^\n]*)\".*" "\\1" ALSA_VERSION "${_ALSA_VERSION_H_CONTENTS}")
#endif(Crypto_INCLUDE_DIR)

set(Crypto_PROCESS_INCLUDES Crypto_INCLUDE_DIR)
set(Crypto_PROCESS_LIBS Crypto_LIBRARY)
libfind_process(Crypto)
