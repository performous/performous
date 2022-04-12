include(LibFindMacros)

set(Aubio_FIND_REQUIRED FALSE)
set(Aubio_FIND_QUIETLY TRUE)
libfind_pkg_detect(Aubio aubio FIND_PATH aubio.h PATH_SUFFIXES aubio FIND_LIBRARY aubio)
libfind_process(Aubio)

if (Aubio_FOUND)
	set(Aubio_VERSION ${Aubio_PKGCONF_VERSION})
else ()
    message("-- Fetching aubio from github...")
    include(FetchContent)
    set(Aubio_VERSION "0.4.9-performous")
    FetchContent_Declare(aubio_from_source
      GIT_REPOSITORY https://github.com/performous/aubio.git
      GIT_SHALLOW    TRUE
      GIT_TAG        14fec3da6749fbcc47b56648d7a38296eccd9499
      SOURCE_DIR aubio-src
    )
    FetchContent_MakeAvailable(aubio_from_source)
    set(Aubio_INCLUDE_DIRS ${aubio_from_source_SOURCE_DIR}/include/)
    set(Aubio_LIBRARIES "-laubio")
    set(Aubio_FOUND TRUE)
endif()
message(STATUS "Found Aubio ${Aubio_VERSION}")
