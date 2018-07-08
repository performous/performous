# -*- cmake -*-

# - Find JSONCpp
# Find the JSONCpp includes and library
# This module defines
#  JSONCPP_INCLUDE_DIRS, where to find json.h, etc.
#  JSONCPP_LIBRARIES, the libraries needed to use jsoncpp.
#  JSONCPP_FOUND, If false, do not try to use jsoncpp.
#  also defined, but not for general use are
#  JSONCPP_LIBRARY, where to find the jsoncpp library.

include(LibFindMacros)
libfind_pkg_detect(JSONCPP jsoncpp FIND_PATH json/json.h FIND_LIBRARY jsoncpp)
set(JSONCPP_VERSION ${JSONCPP_PKGCONF_VERSION})
libfind_process(JSONCPP)

