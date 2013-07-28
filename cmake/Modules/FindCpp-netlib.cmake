# - Try to find CPP-NETLIB
# Once done, this will define
#
#  CPP-NETLIB_FOUND - system has CPP-NETLIB
#  CPP-NETLIB_INCLUDE_DIRS - the CPP-NETLIB include directories
#  CPP-NETLIB_LIBRARIES - link these to use CPP-NETLIB
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)


set(Boost_USE_MULTITHREADED ON)
set(Boost_COMPONENTS system regex date_time filesystem program_options )
find_package( Boost 1.53 REQUIRED ${Boost_COMPONENTS} )

set(CPP-NETLIB_SOURCE_DIR /usr/include/cpp-netlib)
set(CPP-NETLIB_TARGET_DIR /usr/lib/cpp-netlib-build)
set(CPP-NETLIB_COMPONENTS uri message message-directives message-wrappers http-message http-message-wrappers constants http-client http-client-connections )
find_package( CPP-NETLIB REQUIRED ${CPP-NETLIB_COMPONENTS})

#Use find_package to detect other libraries that the library depends on
#find_package(boost REQUIRED)
libfind_package(CPP-NETLIB boost)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(CPP-NETLIB_PKGCONF cpp-netlib)

# Include dir
find_path(CPP-NETLIB_INCLUDE_DIR
  NAMES cpp-netlib.h
  PATHS ${CPP-NETLIB_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(CPP-NETLIB_LIBRARY
  NAMES asound
  PATHS ${CPP-NETLIB_PKGCONF_LIBRARY_DIRS}
)

if(CPP-NETLIB_INCLUDE_DIR)
  # Extract the version number
  file(READ "${ALSA_INCLUDE_DIR}/alsa/version.h" _ALSA_VERSION_H_CONTENTS)
  string(REGEX REPLACE ".*#define SND_LIB_VERSION_STR[ \t]*\"([^\n]*)\".*" "\\1" ALSA_VERSION "${_ALSA_VERSION_H_CONTENTS}")
endif(ALSA_INCLUDE_DIR)

set(CPP-NETLIB_PROCESS_INCLUDES CPP-NETLIB_INCLUDE_DIR) #BOOST_INCLUDE_DIRS
set(CPP-NETLIB_PROCESS_LIBS CPP-NETLIB_LIBRARY) #BOOST_LIBRARIES
libfind_process(CPP-NETLIB)

