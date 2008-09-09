include(CheckIncludeFile)

find_package ( PkgConfig )

if ( PKG_CONFIG_FOUND )
  pkg_check_modules ( PKGCONFIG_LIBRSVG librsvg-2.0 )
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_LIBRSVG_FOUND )
  set ( LIBRSVG_FOUND ${PKGCONFIG_LIBRSVG_FOUND} )
  set ( LIBRSVG_INCLUDE_DIRS ${PKGCONFIG_LIBRSVG_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_LIBRSVG_LIBRARIES} )
    find_library ( ${i}_LIBRARY
      NAMES ${i}
      PATHS ${PKGCONFIG_LIBRSVG_LIBRARY_DIRS}
    )
    if ( ${i}_LIBRARY )
      list ( APPEND LIBRSVG_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  set ( LIBRSVG_LIBRARIES "${LIBRSVG_LIBRARIES}" CACHE STRING "" )
  mark_as_advanced ( LIBRSVG_LIBRARIES )

else ( PKGCONFIG_LIBRSVG_FOUND )
  find_path ( LIBRSVG_INCLUDE_PATH
    NAMES
      librsvg/rsvg.h
    PATHS
      $ENV{LIBRSVG_ROOT_DIR}/include
    PATH_SUFFIXES
      librsvg-2
  )
  mark_as_advanced ( LIBRSVG_INCLUDE_PATH )

  foreach ( i ${LIBRSVG_COMPONENTS} )
    find_library ( ${i}_LIBRARY
      NAMES
        ${i}
      PATHS
        $ENV{LIBRSVG_ROOT_DIR}/lib
    )
    if ( ${i}_LIBRARY )
      list ( APPEND LIBRSVG_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  mark_as_advanced ( LIBRSVG_LIBRARIES )

  set ( LIBRSVG_INCLUDE_DIRS 
    ${LIBRSVG_INCLUDE_PATH}
  )

  if ( LIBRSVG_INCLUDE_DIRS AND LIBRSVG_LIBRARIES )
    set ( LIBRSVG_FOUND true )
  endif ( LIBRSVG_INCLUDE_DIRS AND LIBRSVG_LIBRARIES )
endif ( PKGCONFIG_LIBRSVG_FOUND )

if ( LIBRSVG_FOUND )
  set ( CMAKE_REQUIRED_INCLUDES "${LIBRSVG_INCLUDE_DIRS}" )
  check_include_file ( librsvg/rsvg.h LIBRSVG_FOUND )
endif ( LIBRSVG_FOUND )

if ( NOT LIBRSVG_FOUND )
  if ( NOT LIBRSVG_FIND_QUIETLY )
    message ( STATUS "LIBRSVG not found, try setting LIBRSVG_ROOT_DIR environment variable." )
  endif ( NOT LIBRSVG_FIND_QUIETLY )
  if ( LIBRSVG_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( LIBRSVG_FIND_REQUIRED )
endif ( NOT LIBRSVG_FOUND )

