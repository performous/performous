# - Try to find LibRSVG
# Once done, this will define
#
#  LibRSVG_FOUND - system has LibRSVG
#  LibRSVG_INCLUDE_DIRS - the LibRSVG include directories
#  LibRSVG_LIBRARIES - link these to use LibRSVG

include(LibFindMacros)
libfind_package(LibRSVG Cairo)
libfind_package(LibRSVG GDK-PixBuf)
libfind_pkg_detect(LibRSVG librsvg-2.0
  FIND_PATH librsvg/rsvg.h PATH_SUFFIXES librsvg-2 librsvg-2.0
  FIND_LIBRARY rsvg-2 rsvg-2.40
)
libfind_process(LibRSVG)

