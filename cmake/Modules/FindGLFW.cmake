# - Try to find GLFW
# Once done, this will define
#
#  GLFW_FOUND - system has OpenGL (GL and GLU)
#  GLFW_INCLUDE_DIRS - the OpenGL include directories
#  GLFW_LIBRARIES - link these to use OpenGL
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Dependencies
libfind_package(GDK OpenGL)

libfind_pkg_check_modules(GLFW_PKGCONF libglfw)

find_path(GLFW_INCLUDE_DIR
  NAMES GL/glfw.h
  HINTS ${GLFW_PKGCONF_INCLUDE_DIRS}
)

find_library(GLFW_LIBRARY
  NAMES glfw
  HINTS ${GLFW_PKGCONF_LIBRARY_DIRS}
)

set(GLFW_PROCESS_INCLUDES GLFW_INCLUDE_DIR OpenGL_INCLUDE_DIRS)
set(GLFW_PROCESS_LIBS GLFW_LIBRARY OpenGL_LIBRARIES)

if(UNIX)
  find_library(X11_LIBRARY NAMES X11)
  find_library(Xrandr_LIBRARY NAMES Xrandr)
  set(GLFW_PROCESS_LIBS ${GLFW_PROCESS_LIBS} X11_LIBRARY Xrandr_LIBRARY)
endif(UNIX)

libfind_process(GLFW)

