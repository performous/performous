cmake_minimum_required(VERSION 3.24)

include(LibFindMacros)
include(LibFetchMacros)

set(Spdlog_GIT_VERSION "v1.15.3")

message(STATUS "FMT VERSION: ${fmt_VERSION}")

if(fmt_VERSION VERSION_GREATER_EQUAL 11.1.0)
	set(spdlog_needed "1.15.1")
else()
	set(spdlog_needed "1.12.0")
	set(Spdlog_GIT_VERSION "v1.15.2")
endif()

if(SELF_BUILT_SPDLOG STREQUAL "ALWAYS")
	message(STATUS "spdlog forced to build from source")
	libfetch_git_pkg(Spdlog
		REPOSITORY ${SELF_BUILT_GIT_BASE}/spdlog.git
		REFERENCE  ${Spdlog_GIT_VERSION}
		OVERRIDE_FIND_PACKAGE
	)
elseif(SELF_BUILT_SPDLOG STREQUAL "NEVER")
	find_package(spdlog ${spdlog_needed} REQUIRED PATHS /usr/lib PATH_SUFFIXES ${CMAKE_CXX_LIBRARY_ARCHITECTURE}/cmake/spdlog)
elseif(SELF_BUILT_SPDLOG STREQUAL "AUTO")
	find_package(spdlog ${spdlog_needed} PATHS /usr/lib PATH_SUFFIXES ${CMAKE_CXX_LIBRARY_ARCHITECTURE}/cmake/spdlog)
	if(NOT spdlog_FOUND)
		message(STATUS "spdlog build from source because not found on system")
		libfetch_git_pkg(Spdlog
			REPOSITORY ${SELF_BUILT_GIT_BASE}/spdlog.git
			REFERENCE  ${Spdlog_GIT_VERSION}
			OVERRIDE_FIND_PACKAGE
		)
	else()
	endif()
else()
	message(FATAL_ERROR "unknown SELF_BUILT_SPDLOG value \"${SELF_BUILT_SPDLOG}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

message(STATUS "Found Spdlog ${Spdlog_VERSION}")
