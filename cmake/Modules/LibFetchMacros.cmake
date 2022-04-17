#

include(FetchContent)

function (libfetch_git_pkg PREFIX)
	# parse arguments
	set(argname pkgargs)
	foreach(i ${ARGN})
		if ("${i}" STREQUAL "REPOSITORY")
			set(argname pkgrepository)
		elseif ("${i}" STREQUAL "REFERENCE")
			set(argname pkgreference)
		elseif ("${i}" STREQUAL "NAME")
			set(argname pkgname)
		else ()
			set(${argname} ${${argname}} ${i})
		endif()
	endforeach()

	# FIXME: improve check on REPOSITORY, REFERENCE and NAME
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

	set(${PREFIX}_INCLUDE_DIRS ${${pkgname}_SOURCE_DIR}/include/ PARENT_SCOPE)
	set(${PREFIX}_FOUND TRUE PARENT_SCOPE)
endfunction ()
