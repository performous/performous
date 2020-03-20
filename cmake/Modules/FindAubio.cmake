# - Try to find Aubio
# Once done, this will define
#
#  Aubio_FOUND - system has Aubio
#  Aubio_INCLUDE_DIRS - the Aubio include directories
#  Aubio_LIBRARIES - link these to use Aubio

include(LibFindMacros)
libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
libfind_process(Aubio)