#pragma once

// CMake uses config.cmake.hh to generate config.hh within the build folder.
#ifndef PERFORMOUS_CONFIG_HH
#define PERFORMOUS_CONFIG_HH

#define PACKAGE "@CMAKE_PROJECT_NAME@"
#define VERSION "@PROJECT_VERSION@"

#define SHARED_DATA_DIR "@SHARE_INSTALL@"

// FFMPEG libraries use changing include file names... Get them from CMake.
#define AVCODEC_INCLUDE <@AVCodec_INCLUDE@>
#define AVFORMAT_INCLUDE <@AVFormat_INCLUDE@>
#define SWSCALE_INCLUDE <@SWScale_INCLUDE@>

#endif

