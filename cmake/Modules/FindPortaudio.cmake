# - Try to find Portaudio
# Once done, this will define
#
#  Portaudio_FOUND - system has Portaudio
#  Portaudio_INCLUDE_DIRS - the Portaudio include directories
#  Portaudio_LIBRARIES - link these to use Portaudio
#  Portaudio_VERSION - detected version of Portaudio
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(Portaudio_PKGCONF portaudio-2.0)

if(Portaudio_PKGCONF_FOUND)
  set(Portaudio_VERSION 19)
else(Portaudio_PKGCONF_FOUND)
  set(Portaudio_VERSION 18)
endif(Portaudio_PKGCONF_FOUND)

find_path(Portaudio_INCLUDE_DIR
  NAMES portaudio.h
  PATHS ${Portaudio_PKGCONF_INCLUDE_DIRS}
)

find_library(Portaudio_LIBRARY
  NAMES portaudio
  PATHS ${Portaudio_PKGCONF_LIBRARY_DIRS}
)

set(Portaudio_PROCESS_INCLUDES Portaudio_INCLUDE_DIR)
set(Portaudio_PROCESS_LIBS Portaudio_LIBRARY)
libfind_process(Portaudio)

