# - Try to find ImageMagick++
# Once done, this will define
#
#  Magick++_FOUND - system has Magick++
#  Magick++_INCLUDE_DIRS - the Magick++ include directories
#  Magick++_LIBRARIES - link these to use Magick++

include(LibFindMacros)

# Dependencies
libfind_package(Magick++ Magick)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Magick++_PKGCONF ImageMagick++)

# Include dir
find_path(Magick++_INCLUDE_DIR
  NAMES Magick++.h
  HINTS ${Magick++_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(Magick++_LIBRARY
  NAMES Magick++ CORE_RL_magick++_
  HINTS ${Magick++_PKGCONF_LIBRARY_DIRS}
)

# Process Magick++, depends on Magick
libfind_process(Magick++ Magick)

