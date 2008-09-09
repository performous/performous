include(CheckIncludeFile)

find_package ( PkgConfig )

if ( PKG_CONFIG_FOUND )
  pkg_check_modules ( PKGCONFIG_PANGO pango )
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_PANGO_FOUND )
  set ( PANGO_FOUND ${PKGCONFIG_PANGO_FOUND} )
  set ( PANGO_INCLUDE_DIRS ${PKGCONFIG_PANGO_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_PANGO_LIBRARIES} )
    find_library ( ${i}_LIBRARY
      NAMES ${i}
      PATHS ${PKGCONFIG_PANGO_LIBRARY_DIRS}
    )
    if ( ${i}_LIBRARY )
      list ( APPEND PANGO_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  set ( PANGO_LIBRARIES "${PANGO_LIBRARIES}" CACHE STRING "" )
  mark_as_advanced ( PANGO_LIBRARIES )

else ( PKGCONFIG_PANGO_FOUND )
  find_path ( PANGO_INCLUDE_PATH
    NAMES
      pango/pango.h
    PATHS
      $ENV{PANGO_ROOT_DIR}/include
    PATH_SUFFIXES
      pango-1.0
  )
  mark_as_advanced ( PANGO_INCLUDE_PATH )

  foreach ( i ${PANGO_COMPONENTS} )
    find_library ( ${i}_LIBRARY
      NAMES
        ${i}
      PATHS
        $ENV{PANGO_ROOT_DIR}/lib
    )
    if ( ${i}_LIBRARY )
      list ( APPEND PANGO_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  mark_as_advanced ( PANGO_LIBRARIES )

  set ( PANGO_INCLUDE_DIRS 
    ${PANGO_INCLUDE_PATH}
  )

  if ( PANGO_INCLUDE_DIRS AND PANGO_LIBRARIES )
    set ( PANGO_FOUND true )
  endif ( PANGO_INCLUDE_DIRS AND PANGO_LIBRARIES )
endif ( PKGCONFIG_PANGO_FOUND )

if ( PANGO_FOUND )
  set ( CMAKE_REQUIRED_INCLUDES "${PANGO_INCLUDE_DIRS}" )
  check_include_file ( pango/pango.h PANGO_FOUND )
endif ( PANGO_FOUND )

if ( NOT PANGO_FOUND )
  if ( NOT PANGO_FIND_QUIETLY )
    message ( STATUS "PANGO not found, try setting PANGO_ROOT_DIR environment variable." )
  endif ( NOT PANGO_FIND_QUIETLY )
  if ( PANGO_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( PANGO_FIND_REQUIRED )
endif ( NOT PANGO_FOUND )

