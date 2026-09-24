include(LibFindMacros)
include(LibFetchMacros)

set(Aubio_GIT_VERSION "master")

#set(BLA_VENDOR OpenBLAS)
set(BLA_PKGCONFIG_BLAS openblas)
find_package(BLAS REQUIRED)

# also Accelerate.framework for mac

if (NOT APPLE AND NOT BLAS_LIBRARIES MATCHES "openblas")
	message(FATAL_ERROR "BLAS vendor is not OpenBLAS. Found: ${BLAS_LIBRARIES}")
elseif (APPLE AND NOT BLAS_LIBRARIES MATCHES "Accelerate.framework")
	message(FATAL_ERROR "BLAS vendor is not Accelerate.framework. Found: ${BLAS_LIBRARIES}")
endif()

if(SELF_BUILT_AUBIO STREQUAL "ALWAYS")
	message(STATUS "aubio forced to build from source")
	libfetch_git_pkg(Aubio
		REPOSITORY ${SELF_BUILT_GIT_BASE}/aubio.git
		REFERENCE  ${Aubio_GIT_VERSION}
		#FIND_PATH  aubio/aubio.h
	)

	target_link_libraries(aubio PRIVATE ${BLAS_LIBRARIES})
elseif(SELF_BUILT_AUBIO STREQUAL "NEVER")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(AUBIO REQUIRED QUIET IMPORTED_TARGET GLOBAL aubio>=${Aubio_FIND_VERSION})
	add_library(aubio ALIAS PkgConfig::AUBIO)
	target_link_libraries(PkgConfig::AUBIO INTERFACE ${BLAS_LIBRARIES})
	set(Aubio_VERSION ${AUBIO_VERSION})
	set(Aubio_INCLUDE_DIRS ${AUBIO_INCLUDE_DIRS})
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

		target_link_libraries(aubio PRIVATE ${BLAS_LIBRARIES})
	else()
		add_library(aubio ALIAS PkgConfig::AUBIO)
		target_link_libraries(PkgConfig::AUBIO INTERFACE ${BLAS_LIBRARIES})
		set(Aubio_VERSION ${AUBIO_VERSION})
		set(Aubio_INCLUDE_DIRS ${AUBIO_INCLUDE_DIRS})
	endif()
else()
	message(FATAL_ERROR "unknown SELF_BUILD_AUBIO value \"${SELF_BUILT_AUBIO}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

message(STATUS "Found Aubio ${Aubio_VERSION}")
