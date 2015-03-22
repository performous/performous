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
  HINTS ${Magick_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(Magick_LIBRARY
  NAMES Magick MagickCore CORE_RL_magick_
  HINTS ${Magick_PKGCONF_LIBRARY_DIRS}
)

if(MINGW)
  find_library(MagickWand_LIBRARY
    NAMES MagickWand
    HINTS ${Magick_PKGCONF_LIBRARY_DIRS}
  )
  set(Magick_PROCESS_LIBS MagickWand_LIBRARY)
endif(MINGW)

libfind_version_header(Magick magick/version.h MagickLibVersionText)
libfind_process(Magick)

