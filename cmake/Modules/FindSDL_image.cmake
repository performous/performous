# - Try to find SDL_image
# Once done, this will define
#
#  SDL_image_FOUND - system has SDL_image
#  SDL_image_INCLUDE_DIRS - the SDL include directories
#  SDL_image_LIBRARIES - link these to use SDL_image
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(SDL_image_PKGCONF sdl)

find_path(SDL_image_INCLUDE_DIR
  NAMES SDL_image.h
  PATH_SUFFIXES SDL
  HINTS ${SDL_PKGCONF_INCLUDE_DIRS}
)

find_library(SDL_image_LIBRARY
  NAMES SDL_image
  HINTS ${SDL_PKGCONF_LIBRARY_DIRS}
)

set(SDL_image_PROCESS_INCLUDES SDL_image_INCLUDE_DIR)
set(SDL_image_PROCESS_LIBS SDL_image_LIBRARY)
libfind_process(SDL_image)

