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
libfind_pkg_check_modules(Ssl_PKGCONF openssl)

# Include dir
find_path(Ssl_INCLUDE_DIR
  NAMES openssl/ssl.h
  PATHS ${Ssl_PKGCONF_INCLUDE_DIRS}# /usr/include/
)

# Finally the library itself
find_library(Ssl_LIBRARY
  NAMES ssl
  PATHS ${Ssl_PKGCONF_LIBRARY_DIRS}
)

set(Ssl_PROCESS_INCLUDES Ssl_INCLUDE_DIR)
set(Ssl_PROCESS_LIBS Ssl_LIBRARY)
libfind_process(Ssl)
