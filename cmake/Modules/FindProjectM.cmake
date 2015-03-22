# - Try to find ProjectM
# Once done, this will define
#
#  ProjectM_FOUND - system has ProjectM
#  ProjectM_INCLUDE_DIRS - the ProjectM include directories
#  ProjectM_LIBRARIES - link these to use ProjectM

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(ProjectM_PKGCONF libprojectM)

# Include dir
find_path(ProjectM_INCLUDE_DIR
  NAMES libprojectM/projectM.hpp
  HINTS ${ProjectM_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(ProjectM_LIBRARY
  NAMES projectM
  HINTS ${ProjectM_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(ProjectM_PROCESS_INCLUDES ProjectM_INCLUDE_DIR)
set(ProjectM_PROCESS_LIBS ProjectM_LIBRARY)
libfind_process(ProjectM)

