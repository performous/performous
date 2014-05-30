# - Try to find ALSA
# Once done, this will define
#
#  ALSA_FOUND - system has ALSA
#  ALSA_INCLUDE_DIRS - the ALSA include directories
#  ALSA_LIBRARIES - link these to use ALSA
#  ALSA_VERSION - the library version string
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)
libfind_pkg_detect(ALSA alsa FIND_PATH alsa/version.h FIND_LIBRARY asound)
libfind_version_header(ALSA alsa/version.h SND_LIB_VERSION_STR)
libfind_process(ALSA)

