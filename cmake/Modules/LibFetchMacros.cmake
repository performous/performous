cmake_minimum_required(VERSION 3.11)
include(FetchContent)

# Simple function to abstract fetching a dependency from Git.
# It produces similar output as what libfind_pkg_detect would return:
# - ${PREFIX}_VERSION
# - ${PREFIX}_INCLUDE_DIRS
# - ${PREFIX}_FOUND
# The following attribute are required:
# - PREFIX
# - REPOSITORY <address>: the git repository address
# - REFERENCE <reference>: a git reference for this repository (tag, branch, or hash)
# The following attributes are optional:
# - FIND_PATH <header path>: the path of a file that should be located.
# Eg: libfetch_git_pkg(Json NAME json REPOSITORY https://github.com/performous/json.git REFERENCE master FIND_PATH json/json.hpp)
function (libfetch_git_pkg PREFIX)
	# parse arguments
	set(argname pkgargs)
	foreach(i ${ARGN})
		if ("${i}" STREQUAL "REPOSITORY")
			set(argname pkgrepository)
		elseif ("${i}" STREQUAL "FIND_PATH")
			set(argname pkgfindpath)
		elseif ("${i}" STREQUAL "REFERENCE")
			set(argname pkgreference)
		else ()
			set(${argname} ${${argname}} ${i})
		endif()
	endforeach()

	string(TOLOWER ${PREFIX}-src pkgname)

	if (NOT pkgrepository OR NOT pkgreference)
		message(FATAL_ERROR "libfetch_git_pkg requires attributes REPOSITORY and REFERENCE.")
	endif()
	if (pkgargs)
		message(FATAL_ERROR "libfetch_git_pkg requires no extra parameter.")
	endif()

	message(STATUS "Fetching and making available ${pkgname}...")


	set(${PREFIX}_VERSION ${pkgreference} PARENT_SCOPE)

	FetchContent_Declare(${pkgname}
		GIT_REPOSITORY ${pkgrepository}
		GIT_SHALLOW    TRUE
		GIT_TAG        ${pkgreference}
		SOURCE_DIR     ${pkgname}-src
	)

	FetchContent_MakeAvailable(${pkgname})

	if (pkgfindpath)
		find_path(${PREFIX}_INCLUDE_DIR NAMES ${pkgfindpath} HINTS ${${pkgname}_SOURCE_DIR} ${${pkgname}_SOURCE_DIR}/include)
		set(${PREFIX}_INCLUDE_DIRS ${${PREFIX}_INCLUDE_DIR} PARENT_SCOPE)
	else()
		set(${PREFIX}_INCLUDE_DIRS ${${pkgname}_SOURCE_DIR}/include/ PARENT_SCOPE)
	endif()
	set(${PREFIX}_FOUND TRUE PARENT_SCOPE)
endfunction ()
