# - Try to find ffmpeg
# Once done this will define
#
#  FFMPEG_FOUND - system has ffmpeg
#  FFMPEG_LIBRARIES - Link these to use ffmpeg
#  FFMPEG_DEFINITIONS - Compiler switches required for using ffmpeg

# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (FFMPEG_LIBRARIES)# AND FFMPEG_DEFINITIONS)

  # in cache already
  set(FFMPEG_FOUND TRUE)

else (FFMPEG_LIBRARIES)# AND FFMPEG_DEFINITIONS)
IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)

  PKGCONFIG(libavcodec _FFMPEGIncDir _FFMPEGLinkDir _FFMPEGLinkFlags _FFMPEGCflags)
ENDIF (NOT WIN32)
  #set(FFMPEG_DEFINITIONS ${_FFMPEGCflags} CACHE INTERNAL "The compilation flags for ffmpeg")

  find_path(FFMPEG_INCLUDE_DIR ffmpeg/avcodec.h
    PATHS
    ${_FFMPEGIncDir}
    NO_DEFAULT_PATH
  )

  find_library(AVCODEC_LIBRARIES NAMES avcodec
    PATHS
    ${_FFMPEGLinkDir}
    NO_DEFAULT_PATH
  )

  find_library(AVFORMAT_LIBRARIES NAMES avformat
    PATHS
    ${_FFMPEGLinkDir}
    NO_DEFAULT_PATH
  )

  find_library(AVUTIL_LIBRARIES NAMES avutil
    PATHS
    ${_FFMPEGLinkDir}
    NO_DEFAULT_PATH
  )

  set(FFMPEG_LIBRARIES )
  if (AVCODEC_LIBRARIES)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${AVCODEC_LIBRARIES})
  endif (AVCODEC_LIBRARIES)

  if (AVFORMAT_LIBRARIES)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${AVFORMAT_LIBRARIES})
  endif (AVFORMAT_LIBRARIES)

  if (AVUTIL_LIBRARIES)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${AVUTIL_LIBRARIES})
  endif (AVUTIL_LIBRARIES)

  if (FFMPEG_LIBRARIES)
     set(FFMPEG_FOUND TRUE)
  endif (FFMPEG_LIBRARIES)

  if (FFMPEG_FOUND)
    if (NOT FFmpeg_FIND_QUIETLY)
      message(STATUS "Found FFMPEG: ${FFMPEG_LIBRARIES} ${FFMPEG_INCLUDE_DIR}")
    endif (NOT FFmpeg_FIND_QUIETLY)
  else (FFMPEG_FOUND)
    if (FFmpeg_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find FFMPEG")
    endif (FFmpeg_FIND_REQUIRED)
  endif (FFMPEG_FOUND)

  MARK_AS_ADVANCED(FFMPEG_LIBRARIES)
  MARK_AS_ADVANCED(FFMPEG_INCLUDE_DIR)

endif (FFMPEG_LIBRARIES)# AND FFMPEG_DEFINITIONS)
