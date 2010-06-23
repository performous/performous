# - Try to find OpenGL
# Once done, this will define
#
#  OpenGL_FOUND - system has OpenGL (GL and GLU)
#  OpenGL_INCLUDE_DIRS - the OpenGL include directories
#  OpenGL_LIBRARIES - link these to use OpenGL
#  OpenGL_GL_LIBRARY - only GL
#  OpenGL_GLU_LIBRARY - only GLU
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(OpenGL_PKGCONF gl)

find_path(OpenGL_INCLUDE_DIR
  NAMES GL/gl.h
  PATHS ${OpenGL_PKGCONF_INCLUDE_DIRS}
)

find_library(OpenGL_GL_LIBRARY
  NAMES GL libopengl32.a opengl32
  PATHS ${OpenGL_PKGCONF_LIBRARY_DIRS}
)

find_library(OpenGL_GLU_LIBRARY
  NAMES GLU libglu32.a glu32
  PATHS ${OpenGL_PKGCONF_LIBRARY_DIRS}
)

set(OpenGL_PROCESS_INCLUDES OpenGL_INCLUDE_DIR)
set(OpenGL_PROCESS_LIBS OpenGL_GL_LIBRARY OpenGL_GLU_LIBRARY)
libfind_process(OpenGL)

