cmake_minimum_required(VERSION 2.6)

FILE(GLOB SOURCE_FILES "*.cc")
FILE(GLOB HEADER_FILES "*.hh")

if(WIN32)
  if(ENABLE_VERSIONING)
    set(COMMENTS)
	string(REGEX MATCHALL "([^\\.,^$]+)(\\.|,|$)"  VERSIONING ${PROJECT_VERSION})
	set(VERSIONS "MAJOR" "MINOR" "PATCH" "TWEAK")
	foreach(s ${VERSIONING})
		#we get the most-right number
		string(REGEX REPLACE "[^0-9]+$" "" s ${s})#removes most right part non-numeric (like .)
		#now we have a string in format <numbers-letters..........numbers>
		#take last numbers (if there are)
		#if string not empty, there are some numbers in right of string
		#otherwise, set a 0, to be sure that there's a number
		if(s STREQUAL "")
			set(s "0")
		endif()
		string(REGEX MATCH "[0-9]*$" s ${s})
		
		foreach(v ${VERSIONS})
			if(NOT VERSION_${v}_DONE)
				set(VERSION_${v}_DONE 1)
				set(VERSION_${v} "${s}")
				break()
			endif()
		endforeach()
	endforeach()

	foreach(v ${VERSIONS})
		if(NOT VERSION_${v}_DONE OR VERSION_${v} STREQUAL "") #For the substitution did before, in VERSION_${v} we have or a empty string (if all letters) or a number
			set(VERSION_${v} "0")
		endif()
	endforeach()
	message(STATUS "Setting .exe version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}")
  else(ENABLE_VERSIONING)
    set(COMMENTS "//")
  endif(ENABLE_VERSIONING)
  
  set(RESOURCE_FILES "${CMAKE_BINARY_DIR}/performous.rc")
  set(RESOURCE_OBJECT)#nothing
  
  configure_file("../win32/performous.cmake.rc" "${RESOURCE_FILES}")
  if(NOT MSVC)
    #in according to MinGW tools, we need to compile the rc file, and then link it into projects:
	#windres foo.rc foores.o
    #gcc -o foo.exe foo.o foores.o
	find_program(WINDRES windres)
	if(NOT WINDRES)
		message(STATUS "Cannot find windres. Will not create a versioned exe")
		set(RESOURCE_FILES)
		set(RESOURCE_OBJECT)
	else()
		set(RESOURCE_OBJECT "${CMAKE_BINARY_DIR}/performous.rc.o")
		execute_process(COMMAND "${WINDRES}" "${RESOURCE_FILES}" "${RESOURCE_OBJECT}")
	endif(NOT WINDRES)
  endif()
else()
  set(RESOURCE_FILES) #nothing
  set(RESOURCE_OBJECT)#nothing
endif()

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

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
foreach(lib ${OUR_LIBS} SDL PangoCairo LibRSVG LibXML++ GLEW AVFormat SWScale OpenGL Z Jpeg Png)
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

find_package(OpenCV)
if(OpenCV_FOUND)
	include_directories(${OpenCV_INCLUDE_DIRS})
	list(APPEND LIBS ${OpenCV_LIBRARIES})
	add_definitions("-DUSE_OPENCV")
	message(STATUS "Webcam support: Enabled")
else()
	message(STATUS "Webcam support: Disabled (libcv/libhighgui not found)")
endif()


# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers")
	# -pthread is unrecognized on Windows and ansi standard causes errors
	if(NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -pthread")
	endif(NOT WIN32)
	# Needed for ffmpeg.cc to compile cleanly on OSX (it's a (unsigned long) long story)
	add_definitions("-D__STDC_CONSTANT_MACROS")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(MSVC)
	set(SUBSYSTEM_WIN32 WIN32)
	add_executable(performous-cmd "windows-cmd.cpp")
	install(TARGETS performous-cmd DESTINATION bin)
endif(MSVC)
add_executable(performous ${SUBSYSTEM_WIN32} ${SOURCES} ${SDL_SOURCES} ${RESOURCE_OBJECT})
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

