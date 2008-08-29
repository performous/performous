# - try to find Cairo
# Once done this will define
#
#  CAIRO_FOUND - system has Cairo
#  CAIRO_CFLAGS - the Cairo CFlags
#  CAIRO_LIBRARIES - Link these to use Cairo
#
# Copyright (c) 2007, Pino Toscano, <pino@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (NOT WIN32)
  include(FindPkgConfig)

  PKG_SEARCH_MODULE(CAIRO cairo)
endif(NOT WIN32)

mark_as_advanced(
  CAIRO_INCLUDE_DIR
  CAIRO_LIBRARIES
)

