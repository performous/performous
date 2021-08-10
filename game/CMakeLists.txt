cmake_minimum_required(VERSION 2.6)

file(GLOB SOURCE_FILES "*.cc")
file(GLOB HEADER_FILES "*.hh" "libda/*.hpp")

# Fs is compiled/linked separately to ease switch boost/c++17
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/fs.cc)
list(REMOVE_ITEM HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/fs.hh)

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

# Build main executable
add_executable(performous ${SUBSYSTEM_WIN32} ${SOURCES} ${SDL2_SOURCES})
# Libraries

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_include_directories(performous PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(performous PRIVATE ${Boost_LIBRARIES})

add_library(fs STATIC fs.cc)
target_include_directories(fs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if (USE_BOOST_FS)
    find_package(Boost 1.55 REQUIRED COMPONENTS filesystem)

    message (STATUS "Using boost::filesystem")
    target_link_libraries(fs PUBLIC Boost::filesystem)
    target_compile_definitions(fs PUBLIC -DUSE_BOOST_FS)
else()
    find_package(Filesystem REQUIRED)
    message (STATUS "Using std::filesystem")
    target_link_libraries(fs PRIVATE std::filesystem)
endif()

find_package(ICU 60 REQUIRED uc data i18n io)
target_include_directories(performous SYSTEM PRIVATE ${ICU_INCLUDE_DIRS})
target_link_libraries(performous PRIVATE ${ICU_LIBRARIES})

# LibEpoxy < 1.2 crashes with binary drivers (nvidia & fglrx) when creating shaders
# (see https://github.com/anholt/libepoxy/issues/23 for the exact problem)
find_package(LibEpoxy 1.2 REQUIRED)
target_include_directories(performous SYSTEM PRIVATE ${LibEpoxy_INCLUDE_DIRS})
target_link_libraries(performous PRIVATE ${LibEpoxy_LIBRARIES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gobject-2.0)

# Find all the libs that don't require extra parameters
foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat SWResample SWScale ZLIB JPEG PNG PortAudio Fontconfig GLM)
	find_package(${lib} REQUIRED)
	message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
	target_include_directories(performous SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
	target_link_libraries(performous PRIVATE ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

# Activating MIDI hardware (e-drums with USB/MIDI connection)
if(DEFINED ENABLE_MIDI AND ENABLE_MIDI)
	# Midi support explicitly requested, make it mandatory
	find_package(PortMidi REQUIRED)
	message(STATUS "MIDI I/O support: Enabled (explicitly enabled)")
elseif(DEFINED ENABLE_MIDI AND NOT ENABLE_MIDI)
	# Midi support explicitly disabled
	message(STATUS "MIDI I/O support: Disabled (explicitly disabled)")
else()
	# Midi support not requested explicitly, best effort try to find it
	find_package(PortMidi)
	if(PortMidi_FOUND)
		message(STATUS "MIDI I/O support: Enabled (automatically found)")
	else()
		message(STATUS "MIDI I/O support: Disabled (libportmidi not found)")
	endif()
endif()
# make sure the command line option found is not cached
unset(ENABLE_MIDI CACHE)

if(PortMidi_FOUND)
	include_directories(${PortMidi_INCLUDE_DIRS})
	list(APPEND LIBS ${PortMidi_LIBRARIES})
	add_definitions("-DUSE_PORTMIDI")
endif()

# Activating webcam
if(DEFINED ENABLE_WEBCAM AND ENABLE_WEBCAM)
	# webcam support explicitly requested, make it mandatory
	find_package(OpenCV REQUIRED)
	message(STATUS "Webcam support: Enabled (explicitly enabled)")
elseif(DEFINED ENABLE_WEBCAM AND NOT ENABLE_WEBCAM)
	# webcam support explicitly disabled
	message(STATUS "Webcam support: Disabled (explicitly disabled)")
else()
	# webcam support not requested explicitly, best effort try to find it
	find_package(OpenCV QUIET)
	if(OpenCV_FOUND)
		message(STATUS "Webcam support: Enabled (automatically found)")
	else()
		message(STATUS "Webcam support: Disabled (OpenCV (libcv/libhighgui) not found)")
	endif()
endif()
# make sure the command line option found is not cached
unset(ENABLE_WEBCAM CACHE)

if(OpenCV_FOUND)
	include_directories(${OpenCV_INCLUDE_DIRS})
	list(APPEND LIBS ${OpenCV_LIBS})
	add_definitions("-DUSE_OPENCV")
endif()

# Activating webserver
if(DEFINED ENABLE_WEBSERVER AND ENABLE_WEBSERVER)
	# webserver explicitly requested, make it mandatory
	find_package(CppRest REQUIRED)
	message(STATUS "Webserver support: Enabled (explicitly enabled)")
elseif(DEFINED ENABLE_WEBSERVER AND NOT ENABLE_WEBSERVER)
	# webserver explicitly disabled
	message(STATUS "Webserver support: Disabled (explicitly disabled)")
else()
	# webserver not requested explicitly, best effort try to find it
	find_package(CppRest QUIET)
	if(CppRest_FOUND)
		message(STATUS "Webserver support: Enabled (automatically found)")
	else()
		message(STATUS "Webserver support: Disabled (cpprestsdk not found)")
	endif()
endif()
# make sure the command line option found is not cached
unset(ENABLE_WEBSERVER CACHE)

# if cpprest is found, add it to the dependencies
if(CppRest_FOUND)
	include_directories(${CppRest_INCLUDE_DIRS})
	list(APPEND LIBS ${CppRest_LIBRARIES})
	add_definitions("-DUSE_WEBSERVER")

	# add other required dependencies then
	find_package(Boost 1.55 REQUIRED COMPONENTS chrono thread)
	include_directories(${Boost_INCLUDE_DIRS})
	list(APPEND LIBS ${Boost_LIBRARIES})
endif()

if(WIN32)
	add_definitions("-DEPOXY_SHARED")
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

if(WIN32)
  target_link_libraries(performous PRIVATE wsock32 ws2_32)
endif()

if(APPLE)
	list(APPEND LIBS "-framework Accelerate")
endif()
list(APPEND LIBS ${FFTW3_LIBRARIES})
list(APPEND LIBS ${BLAS_LIBRARIES})

find_package(Threads)

target_link_libraries(performous PRIVATE fs ced aubio ${LIBS} PkgConfig::deps Threads::Threads)

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

