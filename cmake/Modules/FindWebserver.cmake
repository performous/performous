# - Try to find nlohmann_json, if not found, download it from github
# Once done, this will define
#
#  Webserver_FOUND - system has nlohmann_json
#  Webserver_INCLUDE_DIRS - the nlohmann_json include directories
#  Webserver_LIBRARIES - link these to use nlohmann_json
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)
include(LibFetchMacros)

set(Webserver_GIT_VERSION "v0.10.7")

if(SELF_BUILT_WEBSERVER STREQUAL "ALWAYS")
	message(STATUS "Webserver forced to build from source")
	libfetch_git_pkg(Webserver
		REPOSITORY https://github.com/yhirose/cpp-httplib.git
		REFERENCE  ${Webserver_GIT_VERSION}
		FIND_PATH  httplib.h
	)
	message(STATUS "Found Webserver ${Webserver_VERSION}")
elseif(SELF_BUILT_WEBSERVER STREQUAL "NEVER")
	libfind_pkg_detect(Webserver httplib FIND_PATH httplib.h)
	libfind_process(Webserver)
	set(Webserver_VERSION ${Webserver_PKGCONF_VERSION})
elseif(SELF_BUILT_WEBSERVER STREQUAL "AUTO")
	set(Webserver_FIND_REQUIRED FALSE)
	set(Webserver_FIND_QUIETLY TRUE)
	libfind_pkg_detect(Webserver httplib FIND_PATH httplib.h)
	libfind_process(Webserver)
	if(NOT Webserver_FOUND)
		message(STATUS "Webserver build from source because not found on system")
		libfetch_git_pkg(Webserver
			REPOSITORY https://github.com/yhirose/cpp-httplib.git
			REFERENCE  ${Webserver_GIT_VERSION}
			FIND_PATH  httplib.h
		)
	else()
		set(Webserver_VERSION ${Webserver_PKGCONF_VERSION})
	endif()
	message(STATUS "Found Webserver ${Webserver_VERSION}")
else()
	message(FATAL_ERROR "unknown SELF_BUILD_WEBSERVER value \"${SELF_BUILT_WEBSERVER}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()
