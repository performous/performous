# - Try to find nlohmann_json, if not found, download it from github
# Once done, this will define
#
#  RESTinio_FOUND - system has RESTinio
#  RESTinio_INCLUDE_DIRS - the RESTinio include directories
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries


include(LibFindMacros)
include(LibFetchMacros)

set(RESTinio_GIT_VERSION "performous")

if(SELF_BUILT_RESTINIO STREQUAL "ALWAYS")
	message(STATUS "RESTinio forced to build from source")
	cmake_policy(SET CMP0077 NEW) # Ignore option directives from the RESTinio CMakeLists.
	set (RESTINIO_FMT_HEADER_ONLY OFF CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
	set (RESTINIO_PERFORMOUS_HACK ON CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
	set (RESTINIO_FIND_DEPS ON CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
	libfetch_git_pkg(RESTinio
		REPOSITORY ${SELF_BUILT_GIT_BASE}/restinio.git
		REFERENCE  ${RESTinio_GIT_VERSION}
		SOURCE_SUBDIR dev
	)
	set (RESTinio_FOUND TRUE CACHE INTERNAL "Got RESTinio" FORCE)
elseif(SELF_BUILT_RESTINIO STREQUAL "NEVER")
	find_package(restinio REQUIRED CONFIG)
	set(RESTinio_VERSION ${RESTINIO_VERSION})
elseif(SELF_BUILT_RESTINIO STREQUAL "AUTO")
	find_package(restinio QUIET CONFIG)
	if(NOT restinio_FOUND)
		message(STATUS "RESTInio build from source because not found on system")
		cmake_policy(SET CMP0077 NEW) # Ignore option directives from the RESTinio CMakeLists.
		set (RESTINIO_FMT_HEADER_ONLY OFF CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
		set (RESTINIO_PERFORMOUS_HACK ON CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
		set (RESTINIO_FIND_DEPS ON CACHE INTERNAL "Hacks for building as a Performous dependency." FORCE)
		libfetch_git_pkg(RESTinio
			REPOSITORY ${SELF_BUILT_GIT_BASE}/restinio.git
			REFERENCE  ${RESTinio_GIT_VERSION}
			SOURCE_SUBDIR dev
		)
	set (RESTinio_FOUND TRUE CACHE INTERNAL "Got RESTinio" FORCE)
	else()
		set(RESTinio_VERSION ${RESTINIO_VERSION})
	endif()
	message(STATUS "Found RESTinio ${RESTINIO_VERSION}")
else()
	message(FATAL_ERROR "unknown SELF_BUILT_RESTINIO value \"${SELF_BUILT_RESTINIO}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

if (TARGET RESTinio AND (NOT TARGET restinio::restinio))
	message(STATUS "Debug... making restinio alias target")
	add_library(restinio::restinio ALIAS RESTinio)
endif()
