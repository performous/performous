# - Try to find ImageMagick
# Once done, this will define
#
#  Magick_FOUND - system has Magick
#  Magick_INCLUDE_DIRS - the Magick include directories
#  Magick_LIBRARIES - link these to use Magick

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Magick_PKGCONF ImageMagick)

# Include dir
find_path(Magick_INCLUDE_DIR
  NAMES magick/magick.h
  PATHS ${Magick_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(Magick_LIBRARY
  NAMES Magick MagickCore CORE_RL_magick_
  PATHS ${Magick_PKGCONF_LIBRARY_DIRS}
)
set(Magick_PROCESS_LIBS Magick_LIBRARY)

if(MINGW)
  find_library(MagickWand_LIBRARY
    NAMES MagickWand
    PATHS ${Magick_PKGCONF_LIBRARY_DIRS}
  )
  set(Magick_PROCESS_LIBS ${Magick_PROCESS_LIBS} MagickWand_LIBRARY)
endif(MINGW)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Magick_PROCESS_INCLUDES Magick_INCLUDE_DIR)
libfind_process(Magick)

