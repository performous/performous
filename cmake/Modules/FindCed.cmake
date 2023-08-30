include(LibFindMacros)
include(LibFetchMacros)

set(Ced_GIT_VERSION "master")

if(SELF_BUILT_CED STREQUAL "ALWAYS")
	libfetch_git_pkg(Ced
		REPOSITORY ${SELF_BUILT_GIT_BASE}/compact_enc_det.git
		#https://github.com/google/compact_enc_det.git
		REFERENCE  ${Ced_GIT_VERSION}
		FIND_PATH  compact_enc_det/compact_enc_det.h
	)
elseif(SELF_BUILT_CED STREQUAL "NEVER")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(CED REQUIRED IMPORTED_TARGET GLOBAL ced)
    add_library(ced ALIAS PkgConfig::CED)
	set(Ced_VERSION ${CED_VERSION})
	set(Ced_INCLUDE_DIRS ${CED_INCLUDE_DIRS})
elseif(SELF_BUILT_CED STREQUAL "AUTO")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(CED IMPORTED_TARGET GLOBAL CED)
	if(NOT CED_FOUND)
		message(STATUS "ced build from source because not found on system")
		libfetch_git_pkg(Ced
			REPOSITORY ${SELF_BUILT_GIT_BASE}/compact_enc_det.git
			#https://github.com/google/compact_enc_det.git
			REFERENCE  ${Ced_GIT_VERSION}
			FIND_PATH  compact_enc_det/compact_enc_det.h
		)
	else()
		add_library(ced ALIAS PkgConfig::CED)
		set(Ced_VERSION ${CED_VERSION})
		set(Ced_INCLUDE_DIRS ${CED_INCLUDE_DIRS})
	endif()
else()
	message(FATAL_ERROR "unknown SELF_BUILD_CED value \"${SELF_BUILT_CED}\". Allowed values are NEVER, AUTO and ALWAYS")
endif()

message(STATUS "Found Google CED ${Ced_VERSION}")
