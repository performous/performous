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
libfind_pkg_detect(Crypto libcrypto FIND_PATH openssl/crypto.h FIND_LIBRARY crypto)
set(Crypto_VERSION ${Crypto_PKGCONF_VERSION})
libfind_process(Crypto)
