cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

FILE(GLOB SOURCE_FILES "*.cpp")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

include_directories(${CMAKE_SOURCE_DIR}/libda)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
if(APPLE)
	find_package(Boost 1.34 REQUIRED COMPONENTS thread program_options regex filesystem system)
else(APPLE)
	find_package(Boost 1.34 REQUIRED COMPONENTS thread program_options regex filesystem)
endif(APPLE)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL_LIBRARY})
if(APPLE AND ${CMAKE_SYSTEM_VERSION} LESS "9.0.0")
	set(LIBS ${LIBS} SDLmain)
endif(APPLE AND ${CMAKE_SYSTEM_VERSION} LESS "9.0.0")

find_package(LibRSVG REQUIRED)
include_directories(${LIBRSVG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBRSVG_LIBRARIES})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

find_package(Pango REQUIRED)
include_directories(${Pango_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Pango_LIBRARIES})

find_package(LibXML++ REQUIRED)
include_directories(${LibXML++_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LibXML++_LIBRARIES})

find_package(Magick++ REQUIRED)
include_directories(${Magick++_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Magick++_LIBRARIES})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

find_package(FFmpeg REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${FFMPEG_LIBRARIES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # FIXME: this won't work if you use fink on Darwin
	set(LIBS ${LIBS} "/opt/local/lib/libpangocairo-1.0.dylib" "/opt/local/lib/libavutil.dylib") # instead of this use find_library etc
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, enabling warnings")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(performous ${SOURCES})
target_link_libraries(performous da ${LIBS})

install(TARGETS performous DESTINATION bin)

