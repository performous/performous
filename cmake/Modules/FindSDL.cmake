# - Try to find SDL
# Once done, this will define
#
#  SDL_FOUND - system has SDL
#  SDL_INCLUDE_DIRS - the SDL include directories
#  SDL_LIBRARIES - link these to use SDL
#  SDL_SDL_LIBRARY - only libSDL
#  SDL_SDLmain_LIBRARY - only libSDLmain
#  SDL_SOURCES - add this in the source file list of your target (hack for OSX)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(SDL_PKGCONF sdl)

find_path(SDL_INCLUDE_DIR
  NAMES SDL.h
  PATH_SUFFIXES SDL
  HINTS ${SDL_PKGCONF_INCLUDE_DIRS}
)

find_library(SDL_SDL_LIBRARY
  NAMES SDL
  HINTS ${SDL_PKGCONF_LIBRARY_DIRS}
)

# Process others than OSX with native SDL normally
if(NOT "${SDL_SDL_LIBRARY}" MATCHES "framework")
  if(MINGW)
    set(MINGW32_LIBRARY mingw32)
    set(SDL_PROCESS_LIBS ${SDL_PROCESS_LIBS} MINGW32_LIBRARY)
  endif(MINGW)
  find_library(SDL_SDLmain_LIBRARY
    NAMES libSDLmain.a SDLmain
    HINTS ${SDL_PKGCONF_LIBRARY_DIRS}
  )
  if (SDL_SDLmain_LIBRARY)
    set(SDL_PROCESS_LIBS ${SDL_PROCESS_LIBS} SDL_SDLmain_LIBRARY)
  endif (SDL_SDLmain_LIBRARY)
  set(SDL_PROCESS_LIBS ${SDL_PROCESS_LIBS} SDL_SDL_LIBRARY)
endif(NOT "${SDL_SDL_LIBRARY}" MATCHES "framework")

set(SDL_PROCESS_INCLUDES SDL_INCLUDE_DIR)
libfind_process(SDL)

# Special processing for OSX native SDL
if("${SDL_SDL_LIBRARY}" MATCHES "SDL.framework")
  set(SDL_SOURCES "osx/SDLmain.m")
  set(SDL_LIBRARIES "-framework SDL")
 
endif("${SDL_SDL_LIBRARY}" MATCHES "SDL.framework")

# All OSX versions need Cocoa
if(APPLE)
  set(SDL_LIBRARIES ${SDL_LIBRARIES} "-framework Cocoa")
endif(APPLE)

