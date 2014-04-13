# - Try to find libav libavresample
# Once done, this will define
#
#  AVResample_FOUND - the library is available
#  AVResample_INCLUDE_DIRS - the include directories
#  AVResample_LIBRARIES - the libraries
#  AVResample_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibAVFindComponent)

libav_find_component("Resample")

# Handle arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVResample
	DEFAULT_MSG
	AVResample_LIBRARIES
	AVResample_INCLUDE_DIRS
)
if(AVResample_INCLUDE_DIR)
    if(NOT AVResample_INCLUDE)
      if(EXISTS "${AVResample_INCLUDE_DIR}/${suffix}avresample.h")
        set(AVCodec_INCLUDE "${suffix}avcodec.h")
      endif(EXISTS "${AVCodec_INCLUDE_DIR}/${suffix}avresample.h")
    endif(NOT AVResample_INCLUDE)
endif(AVResample_INCLUDE_DIR)

set(AVResample_PROCESS_INCLUDES AVResample_INCLUDE_DIR AVResample_INCLUDE_DIRS)
set(AVResample_PROCESS_LIBS AVResample_LIBRARY AVUtil_LIBRARIES)
libfind_process(AVResample)
mark_as_advanced(AVResample_LIBRARIES AVResample_INCLUDE_DIRS)
