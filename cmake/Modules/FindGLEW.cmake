# - Try to find GLEW
# Once done, this will define
#
#  GLEW_FOUND - system has OpenGL (GL and GLU)
#  GLEW_INCLUDE_DIRS - the OpenGL include directories
#  GLEW_LIBRARIES - link these to use OpenGL
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# GLEW is often installed in the same place as OpenGL
libfind_pkg_check_modules(OpenGL_PKGCONF gl)

find_path(GLEW_INCLUDE_DIR
  NAMES GL/glew.h
  PATHS ${OpenGL_INCLUDE_DIRS}
)

find_library(GLEW_LIBRARY
  NAMES GLEW GLEW32
  PATHS ${OpenGL_PKGCONF_LIBRARY_DIRS}
)

set(GLEW_PROCESS_INCLUDES GLEW_INCLUDE_DIR)
set(GLEW_PROCESS_LIBS GLEW_LIBRARY)
libfind_process(GLEW)

