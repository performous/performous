# - Try to find nlohmann_json, if not found, download it from github
# Once done, this will define
#
#  Json_FOUND - system has nlohmann_json
#  Json_INCLUDE_DIRS - the nlohmann_json include directories
#  Json_LIBRARIES - link these to use nlohmann_json
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)
include(LibFetchMacros)

set(Json_GIT_VERSION "v3.10.5")

if(SELF_BUILT_JSON STREQUAL "ALWAYS")
	message(STATUS "Json forced to build from source")
	libfetch_git_pkg(Json
		NAME       nlohman_json
		REPOSITORY ${SELF_BUILT_GIT_BASE}/json.git
		REFERENCE  ${Json_GIT_VERSION}
	)
	message(STATUS "Found Json ${Json_VERSION}")
elseif(SELF_BUILT_JSON STREQUAL "NEVER")
	libfind_pkg_detect(Json nlohmann_json FIND_PATH nlohmann/json.hpp)
	libfind_process(Json)
	set(Json_VERSION ${Json_PKGCONF_VERSION})
elseif(SELF_BUILT_JSON STREQUAL "AUTO")
	set(Json_FIND_REQUIRED FALSE)
	set(Json_FIND_QUIETLY TRUE)
	libfind_pkg_detect(Json nlohmann_json FIND_PATH nlohmann/json.hpp)
	libfind_process(Json)
	if(NOT Json_FOUND)
		message(STATUS "Json build from source because not found on system")
		libfetch_git_pkg(Json
			NAME       nlohman_json
			REPOSITORY ${SELF_BUILT_GIT_BASE}/json.git
			REFERENCE  ${Json_GIT_VERSION}
		)
	else()
		set(Json_VERSION ${Json_PKGCONF_VERSION})
	endif()
	message(STATUS "Found Json ${Json_VERSION}")
else()
	message(FATAL_ERROR "unknown SELF_BUILD_JSON value \"${SELF_BUILT_JSON}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()
