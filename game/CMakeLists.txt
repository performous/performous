cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

FILE(GLOB SOURCE_FILES "*.cc")
FILE(GLOB HEADER_FILES "*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

include_directories(${CMAKE_SOURCE_DIR}/libs/libda/include)

find_package(Boost 1.34 REQUIRED COMPONENTS thread date_time program_options regex filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

# Find all the libs that don't require extra parameters
foreach(lib SDL PangoCairo LibRSVG LibXML++ Magick++ OpenGL GLEW AVFormat SWScale)
	find_package(${lib} REQUIRED)
	include_directories(${${lib}_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, enabling warnings")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
	# Needed for ffmpeg.cc to compile cleanly on OSX (it's a (unsigned long) long story)
	add_definitions("-D__STDC_CONSTANT_MACROS")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(performous ${SOURCES} ${SDL_SOURCES})
target_link_libraries(performous da ${LIBS})

# Generate config.hh
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS performous DESTINATION bin)

# This should really be in doc/CMakeLists.txt but for some reason the performous target is not visible there
if(UNIX)
	find_program(HELP2MAN help2man DOC "Location of the help2man program")
	find_program(GZIP gzip DOC "Location of the gzip program")
	if(HELP2MAN AND GZIP)
		set(MANFILE ${CMAKE_CURRENT_BINARY_DIR}/performous.1.gz)
		set(H2MFILE ${CMAKE_SOURCE_DIR}/docs/performous.h2m)
		add_custom_command(
			OUTPUT ${MANFILE}
			COMMAND ${HELP2MAN} ${CMAKE_CURRENT_BINARY_DIR}/performous -i ${H2MFILE} -N | ${GZIP} > ${MANFILE}
			MAIN_DEPENDENCY ${H2MFILE}
			DEPENDENCY performous
			COMMENT "Building Performous man page"
			VERBATIM
		)
		add_custom_target(manpage ALL DEPENDS ${MANFILE})
		install(FILES ${MANFILE} DESTINATION share/man/man1)
	else(HELP2MAN AND GZIP)
		message("WARNING: Can't locate help2man or gzip; man pages will not be generated")
	endif(HELP2MAN AND GZIP)
endif(UNIX)

