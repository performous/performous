# MacPorts ships some fast-moving libraries (ffmpeg, opencv, fmt) as major-version-suffixed ports
# (e.g. ffmpeg7, ffmpeg8) that install outside the main /opt/local prefix, since more than one
# ABI-incompatible major version can be installed side by side. Use the newest installed version of each.
#
# Mirrors the same "highest version wins" probing osx-utils/macos-bundler.py already does in Python
# for its own (separately invoked) cmake command line.

function(_performous_highest_versioned_prefix glob_pattern out_var)
	file(GLOB _candidates LIST_DIRECTORIES true ${glob_pattern})
	set(_best_dir "")
	set(_best_version -1)
	foreach(_candidate ${_candidates})
		if(NOT IS_DIRECTORY "${_candidate}")
			continue()
		endif()
		get_filename_component(_name "${_candidate}" NAME)
		string(REGEX MATCH "[0-9]+$" _version "${_name}")
		if(NOT _version)
			set(_version 0)
		endif()
		if(_version GREATER _best_version)
			set(_best_version ${_version})
			set(_best_dir "${_candidate}")
		endif()
	endforeach()
	set(${out_var} "${_best_dir}" PARENT_SCOPE)
endfunction()

if(APPLE AND EXISTS "/opt/local/bin/port")
	_performous_highest_versioned_prefix("/opt/local/libexec/ffmpeg*" _performous_ffmpeg_prefix)
	_performous_highest_versioned_prefix("/opt/local/libexec/opencv*" _performous_opencv_prefix)
	_performous_highest_versioned_prefix("/opt/local/lib/libfmt*" _performous_fmt_prefix)

	if(_performous_ffmpeg_prefix)
		message(STATUS "Detected MacPorts ffmpeg at: ${_performous_ffmpeg_prefix}")
		list(APPEND CMAKE_PREFIX_PATH "${_performous_ffmpeg_prefix}")
	endif()
	if(_performous_opencv_prefix)
		message(STATUS "Detected MacPorts OpenCV at: ${_performous_opencv_prefix}")
		list(APPEND CMAKE_PREFIX_PATH "${_performous_opencv_prefix}")
	endif()
	if(_performous_fmt_prefix)
		message(STATUS "Detected MacPorts fmt at: ${_performous_fmt_prefix}")
		list(APPEND CMAKE_PREFIX_PATH "${_performous_fmt_prefix}/cmake")
	endif()
endif()
