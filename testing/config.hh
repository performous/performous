#pragma once

#define PACKAGE "Performous"
#define VERSION "1.2.3"

#define LOCALE_DIR "share/locale"
#define SHARED_DATA_DIR "share/games/performous"

// FFMPEG/libav libraries use changing include file names... Get them from CMake.
#define AVCODEC_INCLUDE <libavcodec/avcodec.h>
#define AVFORMAT_INCLUDE <libavformat/avformat.h>
#define SWSCALE_INCLUDE <libswscale/swscale.h>
#define SWRESAMPLE_INCLUDE <libswresample/swresample.h>
//libav 0.9 fix
#define AVUTIL_INCLUDE <libavutil/avutil.h>

// libxml++ version
#define LIBXMLPP_VERSION_2_6 1
#define LIBXMLPP_VERSION_3_0 0
#define LIBXMLPP_VERSION_5_0 0
