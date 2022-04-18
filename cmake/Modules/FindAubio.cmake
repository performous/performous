include(LibFindMacros)
include(LibFetchMacros)

set(Aubio_GIT_VERSION "14fec3da6749fbcc47b56648d7a38296eccd9499")

if(SELF_BUILT_AUBIO STREQUAL "ALWAYS")
	message(STATUS "aubio forced to build from source")
	libfetch_git_pkg(Aubio
		NAME aubio
		REPOSITORY ${SELF_BUILT_GIT_BASE}/aubio.git
		REFERENCE  ${Aubio_GIT_VERSION}
	)
	message(STATUS "Found Aubio ${Aubio_VERSION}")
elseif(SELF_BUILT_AUBIO STREQUAL "NEVER")
	libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
	libfind_process(Aubio)
	set(Aubio_VERSION ${Aubio_PKGCONF_VERSION})
elseif(SELF_BUILT_AUBIO STREQUAL "AUTO")
	set(Aubio_FIND_REQUIRED FALSE)
	set(Aubio_FIND_QUIETLY TRUE)
	libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
	libfind_process(Aubio)
	if(NOT Aubio_FOUND)
		message(STATUS "aubio build from source because not found on system")
		libfetch_git_pkg(Aubio
			NAME aubio
			REPOSITORY ${SELF_BUILT_GIT_BASE}/aubio.git
			REFERENCE  ${Aubio_GIT_VERSION}
		)
	else()
		set(Aubio_VERSION ${Aubio_PKGCONF_VERSION})
	endif()
	message(STATUS "Found Aubio ${Aubio_VERSION}")
else()
	message(FATAL_ERROR "unknown SELF_BUILD_AUBIO value \"${SELF_BUILT_AUBIO}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()
