# - Try to find SDL
# Once done, this will define
#
#  SDL_mixer_FOUND - system has SDL
#  SDL_mixer_INCLUDE_DIRS - the SDL include directories
#  SDL_mixer_LIBRARIES - link these to use SDL
#  SDL_mixer_SDL_mixer_LIBRARY - only libSDL
#  SDL_mixer_SDLmain_LIBRARY - only libSDLmain
#  SDL_mixer_SOURCES - add this in the source file list of your target (hack for OSX)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(SDL_mixer_PKGCONF sdl)

find_path(SDL_mixer_INCLUDE_DIR
  NAMES SDL_mixer.h
  PATH_SUFFIXES SDL
  HINTS ${SDL_PKGCONF_INCLUDE_DIRS}
)

find_library(SDL_mixer_LIBRARY
  NAMES SDL_mixer
  HINTS ${SDL_PKGCONF_LIBRARY_DIRS}
)

set(SDL_mixer_PROCESS_INCLUDES SDL_mixer_INCLUDE_DIR)
set(SDL_mixer_PROCESS_LIBS SDL_mixer_LIBRARY)
libfind_process(SDL_mixer)

