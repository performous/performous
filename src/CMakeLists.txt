cmake_minimum_required(VERSION 2.6)

FILE(GLOB SOURCE_FILES "*.cpp")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

include_directories(../audio)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34.0 REQUIRED COMPONENTS thread program_options regex filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL_LIBRARY})

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

add_executable(ultrastarng ${SOURCES})
target_link_libraries(ultrastarng da ${LIBS})

add_definitions("-DTHEMES_DIR=\\\"${SHARE_INSTALL_PREFIX}/themes/\\\"")

install(TARGETS ultrastarng DESTINATION bin)

