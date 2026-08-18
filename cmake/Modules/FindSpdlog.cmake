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

# The FetchContent-vendored spdlog headers can trigger deprecation warnings from a
# newer system fmt than spdlog itself has been updated for (e.g. fmt 12.x's
# deprecated fmt::runtime() string_view conversion, still present as of spdlog
# v1.17.0). Since this project builds with -Werror, that would hard-fail the build
# over a warning in third-party code we don't control. Mark spdlog's headers SYSTEM
# so our own -Werror doesn't apply to warnings coming from inside them.
if(TARGET spdlog_header_only)
	set_target_properties(spdlog_header_only PROPERTIES
		INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:spdlog_header_only,INTERFACE_INCLUDE_DIRECTORIES>")
endif()
