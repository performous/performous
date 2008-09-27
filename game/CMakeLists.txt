cmake_minimum_required(VERSION 2.6)

FILE(GLOB SOURCE_FILES "*.cpp")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

include_directories(${CMAKE_SOURCE_DIR}/libda)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(Boost 1.34 REQUIRED COMPONENTS thread program_options regex filesystem system)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(Boost 1.34 REQUIRED COMPONENTS thread program_options regex filesystem)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL_LIBRARY})
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${CMAKE_SYSTEM_VERSION} LESS "9.0.0")
  set(LIBS ${LIBS} SDLmain)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${CMAKE_SYSTEM_VERSION} LESS "9.0.0")

find_package(LibRSVG REQUIRED)
include_directories(${LIBRSVG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBRSVG_LIBRARIES})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

find_package(Pango REQUIRED)
include_directories(${PANGO_INCLUDE_DIRS})
set(LIBS ${LIBS} ${PANGO_LIBRARIES})

find_package(LibXml++ REQUIRED)
include_directories(${LIBXMLPP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBXMLPP_LIBRARIES})

find_package(Magick++ REQUIRED)
include_directories(${MAGICKPP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${MAGICKPP_LIBRARIES})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(FFmpeg REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${FFMPEG_LIBRARIES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(LIBS ${LIBS} "/opt/local/lib/libpangocairo-1.0.dylib" "/opt/local/lib/libavutil.dylib")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

add_executable(ultrastarng ${SOURCES})
target_link_libraries(ultrastarng da ${LIBS})

# Escaped incorrectly: add_definitions("-DTHEMES_DIR=\\\"${SHARE_INSTALL_PREFIX}/themes/\\\"")

install(TARGETS ultrastarng DESTINATION bin)

