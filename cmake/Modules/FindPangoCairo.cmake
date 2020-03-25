# - Try to find PangoCairo
# Once done, this will define
#
#  PangoCairo_FOUND - system has PangoCairo
#  PangoCairo_LIBRARIES - link these to use PangoCairo

include(LibFindMacros)

libfind_pkg_check_modules(PangoCairo_PKGCONF IMPORTED_TARGET pangocairo)
set(PangoCairo_LIBRARY PkgConfig::PangoCairo_PKGCONF)
libfind_process(PangoCairo)
