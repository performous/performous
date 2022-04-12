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

function(_buildJsonFromSource)
	message(STATUS "Fetching nlohmann_json from github...")
	include(FetchContent)
	set(Json_VERSION "3.10.5")
	set(Json_VERSION "3.10.5" PARENT_SCOPE)
	FetchContent_Declare(nlohmann_json
	  GIT_REPOSITORY https://github.com/performous/json.git
	  GIT_SHALLOW    TRUE
	  GIT_TAG        v${Json_VERSION}
	  SOURCE_DIR     nlohmann_json
	)
	FetchContent_MakeAvailable(nlohmann_json)
	set(Json_INCLUDE_DIRS ${nlohmann_json_SOURCE_DIR}/include/ PARENT_SCOPE)
	set(Json_FOUND TRUE PARENT_SCOPE)
endfunction()

if(FORCE_SELF_BUILT_JSON)
	message(STATUS "Json forced to build from source")
	_buildJsonFromSource()
	message(STATUS "Found Json ${Json_VERSION}")
elseif(NOT ALLOW_SELF_BUILT_JSON)
	libfind_pkg_detect(Json nlohmann_json FIND_PATH nlohmann/json.hpp)
	libfind_process(Json)
	set(Json_VERSION ${Json_PKGCONF_VERSION})
else()
	set(Json_FIND_REQUIRED FALSE)
	set(Json_FIND_QUIETLY TRUE)
	libfind_pkg_detect(Json nlohmann_json FIND_PATH nlohmann/json.hpp)
	libfind_process(Json)
	if(NOT Json_FOUND)
		message(STATUS "Json build from source because not found on system")
		_buildJsonFromSource()
	else()
		set(Json_VERSION ${Json_PKGCONF_VERSION})
	endif()
	message(STATUS "Found Json ${Json_VERSION}")
endif()
