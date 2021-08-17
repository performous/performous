# - Try to find OpenCV
# Once done, this will define
#
#  OpenCV_FOUND - system has OpenCV
#  OpenCV_LIBRARIES - link these to use OpenCV

include(LibFindMacros)

libfind_pkg_check_modules(OpenCV_PKGCONF IMPORTED_TARGET OpenCV)
set(OpenCV_LIBRARY PkgConfig::OpenCV_PKGCONF)
libfind_process(OpenCV)
