cmake_minimum_required(VERSION 3.15)

file(GLOB SOURCE_FILES
	"*.cc"
	"graphic/*.cc"
	"songorder/*.cc"
	"ui/*.cc"
	"ui/layout/*.cc"
	"ui/path/*.cc"
	"ui/effect/*.cc"
	"ui/imagemodifier/*.cc"
	"utils/*.cc"
)
file(GLOB HEADER_FILES
	"*.hh"
	"graphic/*.hh"
	"songorder/*.hh"
	"ui/*.hh"
	"ui/layout/*.hh"
	"ui/path/*.hh"
	"ui/effect/*.hh"
	"ui/imagemodifier/*.hh"
	"utils/*.hh"
	"libda/*.hpp"
)

# Fs is compiled/linked separately to ease switch boost/c++17
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/fs.cc)
list(REMOVE_ITEM HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/fs.hh)

if(WIN32)
	# We want to support all these version numbers:
	# 1.0 1.0.1 1.0+ 1.0.1+ 1.0-2-g123abcd 1.0.1-5-g123abcd
	# We use the 2-3 digits of the version as MAJOR.MINOR.PATCH
	# and the git patch number as TWEAK
	set(SUBSYSTEM_WIN32 WIN32)
	if (DEFINED CMAKE_SOURCE_DIR)
		if(DEFINED ENV{ORIGINAL_TEMP})
			set (TMP_LOCATION $ENV{ORIGINAL_TEMP})
		elseif(DEFINED ENV{TEMP})
			set (TMP_LOCATION $ENV{TEMP})
		else()
			message(WARNING "Temp folder not found. CPack won't be able to create a proper installer.")
		endif()
		file(WRITE "${TMP_LOCATION}/source.txt" "${CMAKE_SOURCE_DIR}")
		file(WRITE "${TMP_LOCATION}/mingwroot.txt" "${CPACK_MINGW_SYSTEM_PREFIX}")
	endif()
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
		if(NOT CMAKE_RC_COMPILER)
			find_program(CMAKE_RC_COMPILER "${CPACK_MINGW_SYSTEM_PREFIX}/bin/windres")
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
target_include_directories(performous SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
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

set(SELF_BUILT_GIT_BASE "https://github.com/performous" CACHE STRING "base path of the git trees used for self built dependencies")
set(SELF_BUILT_AUBIO "AUTO" CACHE STRING "rule to allow self build of Aubio dependency library [NEVER|AUTO*|ALWAYS]")
set(SELF_BUILT_JSON "AUTO" CACHE STRING "rule to allow self build of Json dependency library [NEVER|AUTO*|ALWAYS]")
# This is AUTO cause CED is not available on ubuntu
set(SELF_BUILT_CED "AUTO" CACHE STRING "rule to allow self build of Ced dependency library [NEVER|AUTO*|ALWAYS]")

set(Aubio_REQUIRED_VERSION "0.4.9")

# Find all the libs that don't require extra parameters

if (MINGW)
	target_link_libraries(performous PRIVATE mingw32)
endif()

foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat SWResample SWScale ZLIB JPEG PNG PortAudio Fontconfig GLM Json Ced Aubio)
	find_package(${lib} ${${lib}_REQUIRED_VERSION} REQUIRED)
	message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
	target_include_directories(performous SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
	target_link_libraries(performous PRIVATE ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

find_package(fmt REQUIRED CONFIG)
# CppRESTSDK and their godforsaken U macro.
if(fmt_VERSION VERSION_GREATER_EQUAL 9.0.0)
	target_compile_definitions(performous PRIVATE -D_TURN_OFF_PLATFORM_STRING)
endif()

target_link_libraries(performous PRIVATE fmt::fmt)

target_include_directories(fs PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(fs PRIVATE ${SDL2_LIBRARIES})

# Activating MIDI hardware (e-drums with USB/MIDI connection)
set(ENABLE_MIDI AUTO CACHE STRING "Use midi hardware (e-drums with USB/MIDI connection)")
set_property(CACHE ENABLE_MIDI PROPERTY STRINGS AUTO ON OFF)
if("AUTO" STREQUAL "${ENABLE_MIDI}")
	# Midi support not requested explicitly, best effort try to find it
	find_package(PortMidi QUIET)
	if(PortMidi_FOUND)
		message(STATUS "MIDI I/O support: Enabled (automatically found)")
	else()
		message(STATUS "MIDI I/O support: Disabled (libportmidi not found)")
	endif()
elseif(ENABLE_MIDI)
	# Midi support explicitly requested, make it mandatory
	find_package(PortMidi REQUIRED)
	message(STATUS "MIDI I/O support: Enabled (explicitly enabled)")
else()
	# Midi support explicitly disabled
	message(STATUS "MIDI I/O support: Disabled (explicitly disabled)")
endif()

if(PortMidi_FOUND)
	include_directories(${PortMidi_INCLUDE_DIRS})
	list(APPEND LIBS ${PortMidi_LIBRARIES})
	add_definitions("-DUSE_PORTMIDI")
endif()

# Activating webcam
set(ENABLE_WEBCAM AUTO CACHE STRING "Use webcam")
set_property(CACHE ENABLE_WEBCAM PROPERTY STRINGS AUTO ON OFF)
if("AUTO" STREQUAL "${ENABLE_WEBCAM}")
	# webcam support not requested explicitly, best effort try to find it
	find_package(OpenCV COMPONENTS core videoio)
	if(OPENCV_VIDEOIO_FOUND)
		message(STATUS "Webcam support: Enabled (automatically found)")
	else()
		message(STATUS "Webcam support: Disabled (OpenCV (libcv/libhighgui) not found)")
	endif()
elseif(ENABLE_WEBCAM)
	# webcam support explicitly requested, make it mandatory
	find_package(OpenCV REQUIRED core videoio)
	message(STATUS "Webcam support: Enabled (explicitly enabled)")
else()
	# webcam support explicitly disabled
	message(STATUS "Webcam support: Disabled (explicitly disabled)")
endif()

if(OPENCV_VIDEOIO_FOUND)
	target_include_directories(performous SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
	target_link_libraries(performous PRIVATE ${OpenCV_LIBS})
	add_definitions("-DUSE_OPENCV")
endif()

# Activating webserver
set(ENABLE_WEBSERVER AUTO CACHE STRING "Use webserver")
set_property(CACHE ENABLE_WEBSERVER PROPERTY STRINGS AUTO ON OFF)
if("AUTO" STREQUAL "${ENABLE_WEBSERVER}")
	# webserver not requested explicitly, best effort try to find it
	find_package(cpprestsdk QUIET)
	if (NOT cpprestsdk_FOUND)
		find_package(CppRest QUIET)
	endif()
	if(cpprestsdk_FOUND OR CppRest_FOUND)
		message(STATUS "Webserver support: Enabled (automatically found)")
	else()
		message(STATUS "Webserver support: Disabled (cpprestsdk not found)")
		SET (ENABLE_WEBSERVER OFF FORCE)
	endif()
elseif(ENABLE_WEBSERVER)
	# webserver explicitly requested, make it mandatory
	find_package(cpprestsdk)
	if (NOT cpprestsdk_FOUND)
		find_package(CppRest QUIET REQUIRED)
	endif()
	if(cpprestsdk_FOUND OR CppRest_FOUND)
		message(STATUS "Webserver support: Enabled (explicitly enabled)")
	else()
	# webserver explicitly disabled
		message(STATUS "Webserver support: Disabled (explicitly disabled)")
	endif()
endif()

# if cpprest is found, add it to the dependencies
if(cpprestsdk_FOUND OR CppRest_FOUND)
	# add other required dependencies then
	# FIXME: move this to the CppRest package includes if possible
	find_package(Boost 1.55 REQUIRED COMPONENTS chrono thread)
	find_package(OpenSSL REQUIRED)
	if(cpprestsdk_FOUND)
		target_link_libraries(performous PRIVATE cpprestsdk::cpprest)
	else()
		target_include_directories(performous SYSTEM PRIVATE ${CppRest_INCLUDE_DIRS})
		target_link_libraries(performous PRIVATE ${CppRest_LIBRARIES})
	endif()
	target_link_libraries(performous PRIVATE Boost::boost Boost::chrono OpenSSL::SSL OpenSSL::Crypto)
	add_definitions("-DUSE_WEBSERVER")
endif()

if(WIN32)
	add_definitions("-DEPOXY_SHARED")
	set(BIN_INSTALL .)  # Straight to Program Files/Performous with no bin subfolder.
	if(ENABLE_WEBSERVER)
	  target_link_libraries(performous PRIVATE wsock32 ws2_32)
	endif()
elseif(APPLE)
	set(BIN_INSTALL MacOS)
else()
	set(BIN_INSTALL bin)
endif()

if(APPLE)
	list(APPEND LIBS "-framework Accelerate")
endif()
list(APPEND LIBS ${FFTW3_LIBRARIES})
list(APPEND LIBS ${BLAS_LIBRARIES})

find_package(Threads)
target_link_libraries(performous PRIVATE fs ced aubio ${LIBS} PkgConfig::deps Threads::Threads)

if (NOT APPLE)
	install(TARGETS performous DESTINATION ${BIN_INSTALL})
endif()

if (APPLE AND ${CMAKE_GENERATOR} STREQUAL "Xcode")
	set(PATH "$ENV{PATH}")
	EXECUTE_PROCESS(
		COMMAND /usr/bin/defaults read -g AppleLanguages
		OUTPUT_VARIABLE PERFORMOUS_LANG
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(REGEX REPLACE
	"[ \"\(\)\t\r\n]"
	""
	PERFORMOUS_LANG
	${PERFORMOUS_LANG}
	)
	string(REGEX REPLACE
	"-([A-Z][A-Z]),"
	"_\\1; "
	PERFORMOUS_LANG
	${PERFORMOUS_LANG}
	)
	set(PERFORMOUS_LANG ${PERFORMOUS_LANG})
	list(GET PERFORMOUS_LANG 0 PERFORMOUS_LANG)
	set_target_properties(performous PROPERTIES
	XCODE_SCHEME_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL}/performous"
	XCODE_SCHEME_ARGUMENTS "--log debug"
	XCODE_SCHEME_ENVIRONMENT "FONTCONFIG_PATH=\"${CMAKE_INSTALL_SYSCONFDIR}/fonts\";
	GDK_PIXBUF_MODULE_FILE=\"\";
	LANG=\"${PERFORMOUS_LANG}.UTF-8\""
	)
endif()

set_target_properties(performous PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)  # Store library paths in executable
set_target_properties(performous PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})  # Produce executable in build/, not build/game/

# Capitalized Performous.exe on Windows (this is considered more beautiful).
if(WIN32)
	set_target_properties(performous PROPERTIES OUTPUT_NAME "Performous")
endif()

if(WIN32 AND MSVC)
#	target_compile_options(performous PUBLIC /WX)
else()
	target_compile_options(performous PUBLIC -Werror)
endif()

# Generate config.hh
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32 AND MSVC)
	install(CODE [[
		file(GET_RUNTIME_DEPENDENCIES
			RESOLVED_DEPENDENCIES_VAR deps_resolved
			UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
			EXECUTABLES $<TARGET_FILE:performous>
			PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
			POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
			)
		message(STATUS "Resolving runtime dependencies for $<TARGET_FILE:performous>")
		foreach(dep ${deps_resolved})
			file(INSTALL ${dep} DESTINATION ${CMAKE_INSTALL_PREFIX})
		endforeach()
		foreach(dep ${deps_unresolved})
			message(WARNING "Runtime dependency ${dep} could not be resolved.")
		endforeach()
		]])
elseif(MINGW)
	install(
		CODE [[
		## Hack: CPack doesn't know about the original source folder.
		if(DEFINED ENV{ORIGINAL_TEMP})
			set (TMP_LOCATION $ENV{ORIGINAL_TEMP})
		elseif(DEFINED ENV{TEMP})
			set (TMP_LOCATION $ENV{TEMP})
		else()
			message(WARNING "Temp folder not found. CPack won't be able to create a proper installer.")
		endif()
		file (READ "${TMP_LOCATION}/source.txt" CPACK_TOP_SOURCE_DIR)
		file (READ "${TMP_LOCATION}/mingwroot.txt" CPACK_MINGW_SYSTEM_PREFIX)
		execute_process(
			COMMAND "${CPACK_MINGW_SYSTEM_PREFIX}/../usr/bin/bash.exe" -l -c
			'${CPACK_TOP_SOURCE_DIR}/win32/mingw64/copydlldeps.sh --indir ${CMAKE_INSTALL_PREFIX} --destdir ${CMAKE_INSTALL_PREFIX} --recursivesrcdir ${CPACK_MINGW_SYSTEM_PREFIX} --copy --objdump ${CPACK_MINGW_SYSTEM_PREFIX}/bin/objdump.exe'
			WORKING_DIRECTORY "${CPACK_MINGW_SYSTEM_PREFIX}"
			COMMAND_ECHO STDOUT
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ECHO_OUTPUT_VARIABLE
			OUTPUT_VARIABLE COMMAND_OUTPUT
			ERROR_STRIP_TRAILING_WHITESPACE
			ECHO_ERROR_VARIABLE
			ERROR_VARIABLE COMMAND_OUTPUT
		)
	file(INSTALL
		"${CPACK_MINGW_SYSTEM_PREFIX}/etc/fonts/fonts.conf"
		"${CPACK_MINGW_SYSTEM_PREFIX}/etc/fonts/conf.d"
		DESTINATION "${CMAKE_INSTALL_PREFIX}"
		)
		## Hack: add performous' fonts dir to the config.
		file(READ "${CMAKE_INSTALL_PREFIX}/fonts.conf" FILE_CONTENTS)
		string(REPLACE "<dir>WINDOWSFONTDIR</dir>" "<dir prefix='relative'>./fonts</dir>\n\t<dir>WINDOWSFONTDIR</dir>" FILE_CONTENTS "${FILE_CONTENTS}")
		file(WRITE "${CMAKE_INSTALL_PREFIX}/fonts.conf" "${FILE_CONTENTS}")
		]]
	)
endif()
