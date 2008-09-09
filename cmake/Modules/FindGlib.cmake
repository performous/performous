include(CheckIncludeFile)

set ( GLIB_COMPONENTS glib-2.0 )
foreach ( c ${Glib_FIND_COMPONENTS} )
  list ( APPEND GLIB_COMPONENTS "g${c}-2.0" )
endforeach ( c ) 

find_package ( PkgConfig )

if ( PKG_CONFIG_FOUND )
  pkg_check_modules ( PKGCONFIG_GLIB REQUIRED ${GLIB_COMPONENTS} )
endif ( PKG_CONFIG_FOUND )

if ( PKG_CONFIG_GLIB_FOUND )
  set ( Glib_FOUND ${PKG_CONFIG_GLIB_FOUND} )
  set ( Glib_INCLUDE_DIRS ${PKG_CONFIG_GLIB_INCLUDE_DIRS} )
  foreach ( i ${PKGCONFIG_GLIB_LIBRARIES} )
    find_library ( ${i}_LIBRARY
      NAMES ${i}
      PATHS ${PKGCONFIG_GLIB_LIBRARY_DIRS}
    )
    if ( ${i}_LIBRARY )
      list ( APPEND Glib_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  set ( Glib_LIBRARIES "${Glib_LIBRARIES}" CACHE STRING "" )
  mark_as_advanced ( Glib_LIBRARIES )

else ( PKG_CONFIG_GLIB_FOUND )
  find_path ( GLIB_INCLUDE_PATH
    NAMES
      glib.h
    PATHS
      $ENV{GLIB_ROOT_DIR}/include
    PATH_SUFFIXES
      glib-2.0
  )
  mark_as_advanced ( GLIB_INCLUDE_PATH )

  foreach ( i ${GLIB_COMPONENTS} )
    find_library ( ${i}_LIBRARY
      NAMES
        ${i}
      PATHS
        $ENV{GLIB_ROOT_DIR}/lib
    )
    if ( ${i}_LIBRARY )
      list ( APPEND Glib_LIBRARIES ${${i}_LIBRARY} )
    endif ( ${i}_LIBRARY )
    mark_as_advanced ( ${i}_LIBRARY )
  endforeach ( i )
  mark_as_advanced ( Glib_LIBRARIES )

  if ( glib-2.0_LIBRARY )
    get_filename_component ( glib-2.0_LIBRARY_PATH "${glib-2.0_LIBRARY}" PATH )
    find_path ( GLIBCONFIG_INCLUDE_PATH
      NAMES
        glibconfig.h
      PATHS
        ${glib-2.0_LIBRARY_PATH}/glib-2.0/include   
      NO_DEFAULT_PATH
    )
    mark_as_advanced ( GLIBCONFIG_INCLUDE_PATH )
  endif ( glib-2.0_LIBRARY )

  set ( Glib_INCLUDE_DIRS 
    ${GLIB_INCLUDE_PATH}
    ${GLIBCONFIG_INCLUDE_PATH}
  )

  if ( Glib_INCLUDE_DIRS AND Glib_LIBRARIES )
    set ( Glib_FOUND true )
  endif ( Glib_INCLUDE_DIRS AND Glib_LIBRARIES )
endif ( PKG_CONFIG_GLIB_FOUND )

if ( Glib_FOUND )
  set ( CMAKE_REQUIRED_INCLUDES "${Glib_INCLUDE_DIRS}" )
  check_include_file ( glib.h Glib_FOUND )
endif ( Glib_FOUND )

if ( NOT Glib_FOUND )
  if ( NOT Glib_FIND_QUIETLY )
    message ( STATUS "Glib not found, try setting GLIB_ROOT_DIR environment variable." )
  endif ( NOT Glib_FIND_QUIETLY )
  if ( Glib_FIND_REQUIRED )
    message ( FATAL_ERROR "" )
  endif ( Glib_FIND_REQUIRED )
endif ( NOT Glib_FOUND )

find_program ( GLIB_GENMARSHAL_EXECUTABLE
  NAMES glib-genmarshal
  PATHS ${GLIB2_PREFIX}
  PATH_SUFFIXES bin
)
mark_as_advanced ( GLIB_GENMARSHAL_EXECUTABLE )
