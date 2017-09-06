cmake_minimum_required(VERSION 2.6)

file(GLOB SOURCE_FILES "*.cc" "../common/image.cc")
file(GLOB HEADER_FILES "*.hh" "../common/image.hh" "libda/*.hpp")

if(WIN32)
	# We want to support all these version numbers:
	# 1.0 1.0.1 1.0+ 1.0.1+ 1.0-2-g123abcd 1.0.1-5-g123abcd
	# We use the 2-3 digits of the version as MAJOR.MINOR.PATCH
	# and the git patch number as TWEAK
	string(REGEX REPLACE "\\." ";"  VERSIONING ${PROJECT_VERSION})
	list(GET VERSIONING -1 LAST_ENTRY)
	list(REMOVE_AT VERSIONING -1)
	string(REGEX REPLACE "^([0-9]+)(.*)$" "\\1;\\2"  LAST_ENTRIES ${LAST_ENTRY})
	list(GET LAST_ENTRIES 0 LAST_ENTRY_NUM)
	list(GET LAST_ENTRIES 1 LAST_ENTRY_ADD)
	list(APPEND VERSIONING ${LAST_ENTRY_NUM})

	list(GET VERSIONING 0 VERSION_MAJOR)
	list(LENGTH VERSIONING VERSION_LENGTH)
	if(VERSION_LENGTH GREATER 1)
		list(GET VERSIONING 1 VERSION_MINOR)
	endif()
	if(VERSION_LENGTH GREATER 2)
		list(GET VERSIONING 2 VERSION_PATCH)
	endif()

	string(REGEX REPLACE "^-([0-9]+)-.*$" "\\1" VERSION_TWEAK "${LAST_ENTRY_ADD}")

	set(VERSIONS "MAJOR" "MINOR" "PATCH" "TWEAK")
	foreach(v ${VERSIONS})
		if(NOT VERSION_${v} MATCHES "^[0-9]+$")
			set(VERSION_${v} "0")
		endif()
	endforeach()
	message(STATUS "Setting .exe version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}")

	set(RESOURCE_FILES "${CMAKE_BINARY_DIR}/performous.rc")
	configure_file("../win32/performous.cmake.rc" "${RESOURCE_FILES}")

	if(MINGW)
		# According to MinGW tools, we need to compile the rc file, and then link it into projects:
		# windres foo.rc foores.o
		# gcc -o foo.exe foo.o foores.o
		find_library(WSOCK32_LIBRARY wsock32)
    		find_library(WS2_32_LIBRARY ws2_32)
		if(NOT CMAKE_RC_COMPILER)
			find_program(CMAKE_RC_COMPILER windres)
		endif()
		if(NOT CMAKE_RC_COMPILER)
			message(STATUS "Cannot find windres. Will not create a versioned exe.")
			set(RESOURCE_FILES)
		else()
			set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
		endif()
	endif()
else()
	set(RESOURCE_FILES) #nothing
endif()

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

# Libraries

find_package(Boost 1.36 REQUIRED COMPONENTS thread date_time program_options regex filesystem system locale)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES})

if (Boost_VERSION VERSION_LESS 105500)
	message("-- Using Boost < 1.55: defining BOOST_NO_CXX11_SCOPED_ENUMS")
	set(CMAKE_CXX_FLAGS "-DBOOST_NO_CXX11_SCOPED_ENUMS ${CMAKE_CXX_FLAGS}")
endif()

# LibEpoxy < 1.2 crashes with binary drivers (nvidia & fglrx) when creating shaders
# (see https://github.com/anholt/libepoxy/issues/23 for the exact problem)
find_package(LibEpoxy 1.2 REQUIRED)
include_directories(${LibEpoxy_INCLUDE_DIRS})
list(APPEND LIBS ${LibEpoxy_LIBRARIES})

# Find all the libs that don't require extra parameters
foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat AVResample SWScale Z Jpeg Png PortAudio Fontconfig)
	find_package(${lib} REQUIRED)
	message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
	include_directories(${${lib}_INCLUDE_DIRS})
	list(APPEND LIBS ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

option(ENABLE_MIDI "Enable support for MIDI hardware (e-drums with USB/MIDI connection)." ON)
if(ENABLE_MIDI)
	find_package(PortMidi)
	if(PortMidi_FOUND)
		include_directories(${PortMidi_INCLUDE_DIRS})
		list(APPEND LIBS ${PortMidi_LIBRARIES})
		add_definitions("-DUSE_PORTMIDI")
		message(STATUS "MIDI I/O: Enabled")
	else()
		message(STATUS "MIDI I/O: Disabled (libportmidi not found)")
	endif()
else()
	message(STATUS "MIDI I/O: Disabled (explicitly disabled)")
endif()

option(ENABLE_WEBCAM "Enable webcam support (OpenCV)." ON)
if(ENABLE_WEBCAM)
	find_package(OpenCV)
	if(OpenCV_FOUND)
		include_directories(${OpenCV_INCLUDE_DIRS})
		list(APPEND LIBS ${OpenCV_LIBS})
		add_definitions("-DUSE_OPENCV")
		message(STATUS "Webcam support: Enabled")
	else()
		message(STATUS "Webcam support: Disabled (libcv/libhighgui not found)")
	endif()
else()
	message(STATUS "Webcam support: Disabled (explicitly disabled)")
endif()

option(ENABLE_WEBSERVER "Enable webserver support (cpp-netlib)." ON)
if(ENABLE_WEBSERVER)
	foreach(lib JSONCPP Cppnetlib)
		find_package(${lib})
	endforeach(lib)
	if(JSONCPP_FOUND AND Cppnetlib_FOUND)
		include_directories(${JSONCPP_INCLUDE_DIRS} ${Cppnetlib_INCLUDE_DIRS})
		list(APPEND LIBS ${JSONCPP_LIBRARIES} ${Cppnetlib_LIBRARIES})
		message(STATUS "Webserver support: Enabled")
		add_definitions("-DUSE_WEBSERVER")
	else()
		message(STATUS "Webserver support: Disabled (cpp-netlib or jsoncpp not found)")
	endif()
else()
	message(STATUS "Webserver support: Disabled (explicitly disabled)")
endif()

if(APPLE)
	# Needed for ffmpeg.cc to compile cleanly on OSX (it's a (unsigned long) long story)
	add_definitions("-D__STDC_CONSTANT_MACROS")
endif(APPLE)

if(UNIX)
	# Note: cannot use list APPEND here because it inserts semicolons instead of spaces
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif(UNIX)

if(WIN32)
	option(MXE_HACK "Features horrible hacks, but is able to compile a static performous.exe (that may not work)." OFF)
	mark_as_advanced(MXE_HACK)
	if(MXE_HACK)
		execute_process(COMMAND "${CMAKE_SOURCE_DIR}/win32/mxe/libs.sh"
			OUTPUT_VARIABLE MXE_HACK_STRING
		)
		set(CMAKE_CXX_LINK_EXECUTABLE ${CMAKE_CXX_LINK_EXECUTABLE}${MXE_HACK_STRING})
		add_definitions(-DBOOST_THREAD_USE_LIB)
	endif()
	set(BIN_INSTALL .)  # Straight to Program Files/Performous with no bin subfolder.
	set(SUBSYSTEM_WIN32 WIN32)
else()
	set(BIN_INSTALL bin)
endif()

# Build main executable
add_executable(performous ${SUBSYSTEM_WIN32} ${SOURCES} ${SDL2_SOURCES})
if(WIN32)
  target_link_libraries(performous wsock32 ws2_32)
endif()
target_link_libraries(performous ${LIBS})

install(TARGETS performous DESTINATION ${BIN_INSTALL})

set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)  # Store library paths in executable
set_target_properties(performous PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})  # Produce executable in build/, not build/game/

# Capitalized Performous.exe on Windows (this is considered more beautiful).
if(WIN32)
	set_target_properties(performous PROPERTIES OUTPUT_NAME "Performous")
endif()

# Generate config.hh
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

