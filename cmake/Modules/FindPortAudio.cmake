# - Try to find PortAudio
# Once done, this will define
#
#  PortAudio_FOUND - system has PortAudio
#  PortAudio_INCLUDE_DIRS - the PortAudio include directories
#  PortAudio_LIBRARIES - link these to use PortAudio
#  PortAudio_VERSION - detected version of PortAudio
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(PortAudio_PKGCONF portaudio-2.0)

if(PortAudio_PKGCONF_FOUND)
  set(PortAudio_VERSION 19)
else(PortAudio_PKGCONF_FOUND)
  set(PortAudio_VERSION 18)
endif(PortAudio_PKGCONF_FOUND)

find_path(PortAudio_INCLUDE_DIR
  NAMES portaudio.h
  HINTS ${PortAudio_PKGCONF_INCLUDE_DIRS}
)

find_library(PortAudio_LIBRARY
  NAMES portaudio
  HINTS ${PortAudio_PKGCONF_LIBRARY_DIRS}
)

set(PortAudio_PROCESS_INCLUDES PortAudio_INCLUDE_DIR)
set(PortAudio_PROCESS_LIBS PortAudio_LIBRARY)
libfind_process(PortAudio)

