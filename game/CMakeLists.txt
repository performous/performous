cmake_minimum_required(VERSION 2.6)

FILE(GLOB SOURCE_FILES "*.cc")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

find_package(Boost 1.36 REQUIRED COMPONENTS thread date_time program_options regex filesystem)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES})

# Libraries are part of Performous build process (for now)
set(SLAVE_LIBS TRUE)
if(SLAVE_LIBS)
	list(APPEND LIBS da plugin++)
else()
	set(OUR_LIBS Plugin++ LibDA)
endif()

# Find all the libs that don't require extra parameters
foreach(lib ${OUR_LIBS} SDL PangoCairo LibRSVG LibXML++ Magick++ GLEW AVFormat SWScale OpenGL Z Jpeg Tiff Png)
	find_package(${lib} REQUIRED)
	include_directories(${${lib}_INCLUDE_DIRS})
	list(APPEND LIBS ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

find_package(Gettext)
if(Gettext_FOUND)
	include_directories(${Gettext_INCLUDE_DIRS})
	list(APPEND LIBS ${Gettext_LIBRARIES})
	add_definitions("-DUSE_GETTEXT")
	message(STATUS "Internationalization: Enabled")
else()
	message(STATUS "Internationalization: Disabled (libintl not found)")
endif()

find_package(PortMidi)
if(PortMidi_FOUND)
	include_directories(${PortMidi_INCLUDE_DIRS})
	list(APPEND LIBS ${PortMidi_LIBRARIES})
	add_definitions("-DUSE_PORTMIDI")
	message(STATUS "MIDI I/O: Enabled")
else()
	message(STATUS "MIDI I/O: Disabled (libportmidi not found)")
endif()

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++98 -Wall -Wextra -Wno-missing-field-initializers")
	# Needed for ffmpeg.cc to compile cleanly on OSX (it's a (unsigned long) long story)
	add_definitions("-D__STDC_CONSTANT_MACROS")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(performous ${SOURCES} ${SDL_SOURCES})
target_link_libraries(performous ${LIBS})

# Store library paths in executable
if(APPLE)
	set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
	set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Generate config.hh
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS performous DESTINATION bin)

