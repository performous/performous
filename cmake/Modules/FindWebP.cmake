
# - Try to find WebP
# Once done, this will define
#
#  WebP_FOUND - system has WebP
#  WebP_INCLUDE_DIRS - the WebP include directories
#  WebP_LIBRARIES - link these to use WebP
#  WebP_VERSION - version of WebP found

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(WebP_PKGCONF libwebp)

# Find the main WebP library
find_library(WebP_LIBRARY
  NAMES webp
  HINTS ${WebP_PKGCONF_LIBRARY_DIRS}
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /sw/lib
  PATH_SUFFIXES
    x86_64-linux-gnu
    i386-linux-gnu
    arm-linux-gnueabihf
    aarch64-linux-gnu
)

# Find WebP include directory
find_path(WebP_INCLUDE_DIR
  NAMES webp/encode.h webp/decode.h
  HINTS ${WebP_PKGCONF_INCLUDE_DIRS}
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /sw/include
)

# Find optional WebP libraries (demux, mux)
find_library(WebP_DEMUX_LIBRARY
  NAMES webpdemux
  HINTS ${WebP_PKGCONF_LIBRARY_DIRS}
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /sw/lib
  PATH_SUFFIXES
    x86_64-linux-gnu
    i386-linux-gnu
    arm-linux-gnueabihf
    aarch64-linux-gnu
)

find_library(WebP_MUX_LIBRARY
  NAMES webpmux
  HINTS ${WebP_PKGCONF_LIBRARY_DIRS}
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /sw/lib
  PATH_SUFFIXES
    x86_64-linux-gnu
    i386-linux-gnu
    arm-linux-gnueabihf
    aarch64-linux-gnu
)

# Version detection
if(WebP_PKGCONF_VERSION)
  set(WebP_VERSION ${WebP_PKGCONF_VERSION})
else()
  # Fallback: try to get version from command line tool
  find_program(WEBP_CWEBP_EXECUTABLE NAMES cwebp)
  if(WEBP_CWEBP_EXECUTABLE)
    execute_process(
      COMMAND ${WEBP_CWEBP_EXECUTABLE} -version
      OUTPUT_VARIABLE WEBP_VERSION_OUTPUT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(WEBP_VERSION_OUTPUT MATCHES "([0-9]+\\.[0-9]+\\.[0-9]+)")
      set(WebP_VERSION ${CMAKE_MATCH_1})
    endif()
  endif()
endif()

# Set up the library list
set(WebP_PROCESS_INCLUDES WebP_INCLUDE_DIR)
set(WebP_PROCESS_LIBS WebP_LIBRARY)

# Add optional libraries if found
if(WebP_DEMUX_LIBRARY)
  list(APPEND WebP_PROCESS_LIBS WebP_DEMUX_LIBRARY)
endif()

if(WebP_MUX_LIBRARY)
  list(APPEND WebP_PROCESS_LIBS WebP_MUX_LIBRARY)
endif()

libfind_process(WebP)
