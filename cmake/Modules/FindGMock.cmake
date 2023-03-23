# This content copied from
# https://git.simply-life.net/simply-life.net/talltower/-/blob/639293a366da43eb94a72d2e7596242314c9809c/cmake/FindGMock.cmake


# Try to find GMock
find_package(GTest)

# the following issues a warning, but it works nonetheless
find_package(PkgConfig)
pkg_check_modules(PC_GMOCK QUIET gmock)
set(GMOCK_DEFINITIONS ${PC_GMOCK_CFLAGS_OTHER})

find_path(GMOCK_INCLUDE_DIR gmock.h
          HINTS ${PC_GMOCK_INCLUDEDIR} ${PC_GMOCK_INCLUDE_DIRS}
          PATH_SUFFIXES gmock)

find_library(GMOCK_LIBRARY NAMES gmock libgmock
             HINTS ${PC_GMOCK_LIBDIR} ${PC_GMOCK_LIBRARY_DIRS} )

find_library(GMOCK_MAIN_LIBRARY NAMES gmock_main libgmock_main
             HINTS ${PC_GMOCK_LIBDIR} ${PC_GMOCK_LIBRARY_DIRS} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GMOCK_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GMock DEFAULT_MSG
                                  GMOCK_LIBRARY GMOCK_INCLUDE_DIR GTEST_FOUND)

mark_as_advanced(GMOCK_INCLUDE_DIR GMOCK_LIBRARY GMOCK_MAIN_LIBRARY)

set(GMOCK_LIBRARIES ${GMOCK_LIBRARY} )
set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR} )
set(GMOCK_MAIN_LIBRARIES ${GMOCK_MAIN_LIBRARY} )

if (NOT TARGET GMock)
    add_library(GMock IMPORTED SHARED)
    set_property(TARGET GMock PROPERTY IMPORTED_LOCATION ${GMOCK_LIBRARY})
    set_property(TARGET GMock PROPERTY INTERFACE_INCLUDE_DIRECTORY ${GMOCK_INCLUDE_DIR})

    add_library(GMockMain IMPORTED SHARED)
    set_property(TARGET GMockMain PROPERTY IMPORTED_LOCATION ${GMOCK_MAIN_LIBRARY})
    set_property(TARGET GMockMain PROPERTY INTERFACE_LINK_LIBRARIES GMock GTest)
endif()
