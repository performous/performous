# - Try to find cpp-httplib, if not found, download it from github
# Once done, this will define
#
#  HttpLib_FOUND - system has cpp-httplib
#  HttpLib_INCLUDE_DIRS - the cpp-httplib include directories
#  HttpLib_LIBRARIES - libraries to link (empty for header-only usage)
#  HttpLib_DEFINITIONS - compile definitions required to match the library ABI
#
# cpp-httplib is a single-header HTTP library, but some distros (Debian/Ubuntu)
# ship it precompiled as a shared library built with specific CPPHTTPLIB_*
# defines. When linking against such a library, the same defines must be used
# when including the header, otherwise the ABI will not match.
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)
include(LibFetchMacros)
find_package(Threads QUIET)

set(HttpLib_GIT_VERSION "v0.67.0")

if(SELF_BUILT_HTTPLIB STREQUAL "ALWAYS")
	message(STATUS "HttpLib forced to build from source")
	# cpp-httplib is fetched from upstream (yhirose) rather than the
	# SELF_BUILT_GIT_BASE mirror: the project ships no patches of its own
	# and the performous fork is stale (last tag v0.10.7).
	libfetch_git_pkg(HttpLib
		REPOSITORY https://github.com/yhirose/cpp-httplib.git
		REFERENCE  ${HttpLib_GIT_VERSION}
		FIND_PATH  httplib.h
	)
	message(STATUS "Found HttpLib ${HttpLib_VERSION}")
elseif(SELF_BUILT_HTTPLIB STREQUAL "NEVER")
	# Try a CMake package config first (vcpkg / Fedora provide one), then pkg-config.
	find_package(httplib CONFIG QUIET)
	if(httplib_FOUND)
		set(HttpLib_FOUND TRUE)
		set(HttpLib_LIBRARIES httplib::httplib)
		if(TARGET httplib::httplib)
			get_target_property(HttpLib_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
		endif()
	else()
		libfind_pkg_detect(HttpLib cpp-httplib FIND_PATH httplib.h FIND_LIBRARY cpp-httplib)
		libfind_process(HttpLib)
		if(HttpLib_FOUND AND HttpLib_PKGCONF_FOUND)
			set(HttpLib_DEFINITIONS ${HttpLib_PKGCONF_CFLAGS_OTHER})
		endif()
	endif()
elseif(SELF_BUILT_HTTPLIB STREQUAL "AUTO")
	find_package(httplib CONFIG QUIET)
	if(httplib_FOUND)
		set(HttpLib_FOUND TRUE)
		set(HttpLib_LIBRARIES httplib::httplib)
		if(TARGET httplib::httplib)
			get_target_property(HttpLib_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
		endif()
		message(STATUS "Found HttpLib (config)")
	else()
		set(HttpLib_FIND_REQUIRED FALSE)
		set(HttpLib_FIND_QUIETLY TRUE)
		libfind_pkg_detect(HttpLib cpp-httplib FIND_PATH httplib.h FIND_LIBRARY cpp-httplib)
		libfind_process(HttpLib)
		if(HttpLib_FOUND AND HttpLib_PKGCONF_FOUND)
			set(HttpLib_DEFINITIONS ${HttpLib_PKGCONF_CFLAGS_OTHER})
		endif()
		if(NOT HttpLib_FOUND)
			# AUTO must stay best-effort like the old cpprestsdk detection:
			# never download at configure time, just report not found so the
			# caller can disable webserver support. Use SELF_BUILT_HTTPLIB=ALWAYS
			# to explicitly opt into fetching from source.
			message(STATUS "HttpLib not found on system; webserver support will be disabled "
				"(set SELF_BUILT_HTTPLIB=ALWAYS to fetch cpp-httplib from upstream)")
		endif()
		message(STATUS "Found HttpLib ${HttpLib_VERSION}")
	endif()
else()
	message(FATAL_ERROR "unknown SELF_BUILT_HTTPLIB value \"${SELF_BUILT_HTTPLIB}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

if(HttpLib_FOUND)
	# cpp-httplib's server uses std::thread; ensure pthread is linked on POSIX.
	if(THREADS_FOUND)
		list(APPEND HttpLib_LIBRARIES Threads::Threads)
	endif()
endif()
