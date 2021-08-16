#pragma once

// CMake uses config.cmake.hh to generate config.hh within the build folder.
#ifndef PERFORMOUS_CONFIG_HH
#define PERFORMOUS_CONFIG_HH

#define LOCALEDIR "@LOCALE_DIR@"

#define PACKAGE "@CMAKE_PROJECT_NAME@"
#define VERSION "@PROJECT_VERSION@"

#define SHARED_DATA_DIR "@SHARE_INSTALL@"

// FFMPEG/libav libraries use changing include file names... Get them from CMake.
#define AVCODEC_INCLUDE <@AVCodec_INCLUDE@>
#define AVFORMAT_INCLUDE <@AVFormat_INCLUDE@>
#define SWSCALE_INCLUDE <@SWScale_INCLUDE@>
#define SWRESAMPLE_INCLUDE <@SWResample_INCLUDE@>
//libav 0.9 fix
#define AVUTIL_INCLUDE <@AVUtil_INCLUDE@>
#define AVUTIL_OPT_INCLUDE <@AVUtil_INCLUDE_DIRS@/libavutil/opt.h> //HACK to get AVOption class!
#define AVUTIL_MATH_INCLUDE <@AVUtil_INCLUDE_DIRS@/libavutil/mathematics.h>
#define AVUTIL_ERROR_INCLUDE <@AVUtil_INCLUDE_DIRS@/libavutil/error.h>

// libxml++ version
#define LIBXMLPP_VERSION_2_6 @LibXML++_VERSION_2_6@
#define LIBXMLPP_VERSION_3_0 @LibXML++_VERSION_3_0@
#define LIBXMLPP_VERSION_5_0 @LibXML++_VERSION_5_0@

#endif

