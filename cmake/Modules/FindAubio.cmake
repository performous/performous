include(LibFindMacros)
include(LibFetchMacros)

set(Aubio_GIT_VERSION "master")

if(SELF_BUILT_AUBIO STREQUAL "ALWAYS")
	message(STATUS "aubio forced to build from source")
	libfetch_git_pkg(Aubio
		REPOSITORY ${SELF_BUILT_GIT_BASE}/aubio.git
		REFERENCE  ${Aubio_GIT_VERSION}
		#FIND_PATH  aubio/aubio.h
	)
	set (Aubio_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/aubio-src-build/include" CACHE INTERNAL "Aubio includes" FORCE)
elseif(SELF_BUILT_AUBIO STREQUAL "NEVER")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(AUBIO REQUIRED QUIET IMPORTED_TARGET GLOBAL aubio>=${Aubio_FIND_VERSION})
	add_library(aubio ALIAS PkgConfig::AUBIO)
	set(Aubio_VERSION ${AUBIO_VERSION})
elseif(SELF_BUILT_AUBIO STREQUAL "AUTO")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(AUBIO QUIET IMPORTED_TARGET GLOBAL aubio>=${Aubio_FIND_VERSION})
	if(NOT AUBIO_FOUND)
		message(STATUS "aubio build from source because not found on system")
		libfetch_git_pkg(Aubio
			REPOSITORY ${SELF_BUILT_GIT_BASE}/aubio.git
			REFERENCE  ${Aubio_GIT_VERSION}
			#FIND_PATH  aubio/aubio.h
		)
		set (Aubio_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/aubio-src-build/include" CACHE INTERNAL "Aubio includes" FORCE)
	else()
		add_library(aubio ALIAS PkgConfig::AUBIO)
		set(Aubio_VERSION ${AUBIO_VERSION})
	endif()
else()
	message(FATAL_ERROR "unknown SELF_BUILD_AUBIO value \"${SELF_BUILT_AUBIO}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

message(STATUS "Found Aubio ${Aubio_VERSION}")
