include(LibFindMacros)

function(_buildAubioFromSource)
	message(STATUS "Fetching aubio from github...")
	include(FetchContent)
	set(Aubio_VERSION "0.4.9-performous" PARENT_SCOPE)
	FetchContent_Declare(aubio_from_source
		GIT_REPOSITORY https://github.com/performous/aubio.git
		GIT_SHALLOW    TRUE
		GIT_TAG        14fec3da6749fbcc47b56648d7a38296eccd9499
		SOURCE_DIR aubio-src
	)
	FetchContent_MakeAvailable(aubio_from_source)
	set(Aubio_INCLUDE_DIRS ${aubio_from_source_SOURCE_DIR}/include/ PARENT_SCOPE)
	#set(Aubio_LIBRARIES "-laubio" PARENT_SCOPE)
	set(Aubio_FOUND TRUE PARENT_SCOPE)
endfunction()

if(FORCE_SELF_BUILT_AUBIO)
	message(STATUS "aubio forced to build from source")
	_buildAubioFromSource()
	message(STATUS "Found Aubio ${Aubio_VERSION}")
elseif(NOT ALLOW_SELF_BUILT_AUBIO)
	libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
	libfind_process(Aubio)
	set(Aubio_VERSION ${Aubio_PKGCONF_VERSION})
else()
	set(Aubio_FIND_REQUIRED FALSE)
	set(Aubio_FIND_QUIETLY TRUE)
	libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
	libfind_process(Aubio)
	if(NOT Aubio_FOUND)
		message(STATUS "aubio build from source because not found on system")
		_buildAubioFromSource()
	else()
		set(Aubio_VERSION ${Aubio_PKGCONF_VERSION})
	endif()
	message(STATUS "Found Aubio ${Aubio_VERSION}")
endif()
