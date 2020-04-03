# - Try to find Pango
# Once done, this will define
#
#  Pango_FOUND - system has Pango
#  Pango_LIBRARIES - link these to use Pango

include(LibFindMacros)

libfind_pkg_check_modules(Pango_PKGCONF IMPORTED_TARGET pango)
set(Pango_LIBRARY PkgConfig::Pango_PKGCONF)
libfind_process(Pango)
