cmake_minimum_required(VERSION 2.6)

FILE(GLOB SOURCE_FILES "*.cc")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

find_package(Boost 1.36 REQUIRED COMPONENTS thread date_time program_options regex filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

# Find all the libs that don't require extra parameters
foreach(lib SDL PangoCairo LibRSVG LibXML++ Magick++ GLEW AVFormat SWScale OpenGL)
	find_package(${lib} REQUIRED)
	include_directories(${${lib}_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

find_package(PortMidi)
if(PortMidi_FOUND)
	include_directories(PortMidi_INCLUDE_DIRS)
	set(LIBS ${LIBS} ${PortMidi_LIBRARIES})
	add_definitions("-DHAVE_PORTMIDI")
endif(PortMidi_FOUND)

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++98 -Wall -Wextra")
	# Needed for ffmpeg.cc to compile cleanly on OSX (it's a (unsigned long) long story)
	add_definitions("-D__STDC_CONSTANT_MACROS")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(performous ${SOURCES} ${SDL_SOURCES})
target_link_libraries(performous da ${LIBS})

# Store library paths in executable
if(APPLE)
	set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE INSTALL_RPATH @loader_path/../lib)
else()
	set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE INSTALL_RPATH \$ORIGIN/../lib)
endif()

# Generate config.hh
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS performous DESTINATION bin)

