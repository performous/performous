# - Try to find Cairo
# Once done, this will define
#
#  Cairo_FOUND - system has Cairo
#  Cairo_INCLUDE_DIRS - the Cairo include directories
#  Cairo_LIBRARIES - link these to use Cairo

include(LibFindMacros)
libfind_package(Cairo Freetype)  # Cairo depends on Freetype
libfind_pkg_detect(Cairo cairo FIND_PATH cairo.h PATH_SUFFIXES cairo FIND_LIBRARY cairo)
libfind_process(Cairo)



