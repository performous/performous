# - Try to find Harfbuzz
# Once done, this will define
#
#  Harfbuzz_FOUND - system has Harfbuzz
#  Harfbuzz_INCLUDE_DIRS - the Harfbuzz include directories
#  Harfbuzz_LIBRARIES - link these to use Harfbuzz

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Harfbuzz_PKGCONF harfbuzz)

# Include dir
find_path(Harfbuzz_INCLUDE_DIR
  NAMES harfbuzz/hb.h
  HINTS ${Harfbuzz_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES harfbuzz
)

# Finally the library itself
find_library(Harfbuzz_LIBRARY
  NAMES harfbuzz
  HINTS ${Harfbuzz_PKGCONF_LIBRARY_DIRS}
)

libfind_process(Harfbuzz)

