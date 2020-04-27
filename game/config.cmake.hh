#pragma once

// CMake uses config.cmake.hh to generate config.hh within the build folder.
#ifndef PERFORMOUS_CONFIG_HH
#define PERFORMOUS_CONFIG_HH

#define LOCALEDIR "@LOCALE_DIR@"

#define PACKAGE "@CMAKE_PROJECT_NAME@"
#define VERSION "@PROJECT_VERSION@"

#define SHARED_DATA_DIR "@SHARE_INSTALL@"

// libxml++ version
#define LIBXMLPP_VERSION_2_6 @LibXML++_VERSION_2_6@
#define LIBXMLPP_VERSION_3_0 @LibXML++_VERSION_3_0@

#endif

