# - Try to find LibRSVG
# Once done, this will define
#
#  LibRSVG_FOUND - system has LibRSVG
#  LibRSVG_INCLUDE_DIRS - the LibRSVG include directories
#  LibRSVG_LIBRARIES - link these to use LibRSVG

include(LibFindMacros)

# Detect Rust-built librsvg on Windows first
if(MSVC AND EXISTS "${CMAKE_SOURCE_DIR}/deps/librsvg/librsvg-2.dll")
    message(STATUS "Using bundled Rust-built librsvg")
    set(LibRSVG_FOUND TRUE)
    set(LibRSVG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/librsvg/include")
    set(LibRSVG_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/librsvg/librsvg-2.dll")
else()
    # Existing fallback to system/vcpkg
    libfind_package(LibRSVG Cairo)
    libfind_package(LibRSVG GDK-PixBuf)
    libfind_pkg_detect(LibRSVG librsvg-2.0
        FIND_PATH librsvg/rsvg.h PATH_SUFFIXES librsvg-2 librsvg-2.0
        FIND_LIBRARY rsvg-2 rsvg-2.40
    )
    libfind_process(LibRSVG)
endif()

