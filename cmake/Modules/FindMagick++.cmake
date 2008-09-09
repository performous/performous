include(CheckIncludeFile)

find_package ( PkgConfig )

if ( PKG_CONFIG_FOUND )
  pkg_check_modules ( PKGCONFIG_MAGICKPP ImageMagick++ )
endif ( PKG_CONFIG_FOUND )

if ( PKGCONFIG_MAGICKPP_FOUND )
  set ( MAGICKPP_FOUND ${PKGCONFIG_MAGICKPP_FOUND} )
  set ( MAGICKPP_INCLUDE_DIRS ${PKGCONFIG_MAGICKPP_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_MAGICKPP_LIBRARIES} )
    find_library ( ${i}_LIBRARY
      NAMES ${i}
      PATHS ${PKGCONFIG_MAGICKPP_LIBRARY_DIRS}
    )
    if ( ${i}_LIBRARY )
      list ( APPEND MAGICKPP_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  set ( MAGICKPP_LIBRARIES "${MAGICKPP_LIBRARIES}" CACHE STRING "" )
  mark_as_advanced ( MAGICKPP_LIBRARIES )

else ( PKGCONFIG_MAGICKPP_FOUND )
  find_path ( MAGICKPP_INCLUDE_PATH
    NAMES
      librsvg/rsvg.h
    PATHS
      $ENV{MAGICKPP_ROOT_DIR}/include
    PATH_SUFFIXES
      librsvg-2
  )
  mark_as_advanced ( MAGICKPP_INCLUDE_PATH )

  foreach ( i ${MAGICKPP_COMPONENTS} )
    find_library ( ${i}_LIBRARY
      NAMES
        ${i}
      PATHS
        $ENV{MAGICKPP_ROOT_DIR}/lib
    )
    if ( ${i}_LIBRARY )
      list ( APPEND MAGICKPP_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  mark_as_advanced ( MAGICKPP_LIBRARIES )

  set ( MAGICKPP_INCLUDE_DIRS 
    ${MAGICKPP_INCLUDE_PATH}
  )

  if ( MAGICKPP_INCLUDE_DIRS AND MAGICKPP_LIBRARIES )
    set ( MAGICKPP_FOUND true )
  endif ( MAGICKPP_INCLUDE_DIRS AND MAGICKPP_LIBRARIES )
endif ( PKGCONFIG_MAGICKPP_FOUND )

if ( MAGICKPP_FOUND )
  set ( CMAKE_REQUIRED_INCLUDES "${MAGICKPP_INCLUDE_DIRS}" )
  check_include_file ( librsvg/rsvg.h MAGICKPP_FOUND )
endif ( MAGICKPP_FOUND )

if ( NOT MAGICKPP_FOUND )
  if ( NOT MAGICKPP_FIND_QUIETLY )
    message ( STATUS "MAGICKPP not found, try setting MAGICKPP_ROOT_DIR environment variable." )
  endif ( NOT MAGICKPP_FIND_QUIETLY )
  if ( MAGICKPP_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( MAGICKPP_FIND_REQUIRED )
endif ( NOT MAGICKPP_FOUND )

