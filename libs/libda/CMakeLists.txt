project(libda)
cmake_minimum_required(VERSION 2.6)

set(LIBDA_PLUGIN_DIR "lib${LIB_SUFFIX}/libda")

include_directories(include)
include_directories(../plugin++/include)

FILE(GLOB HEADER_FILES "include/libda/*")
install(FILES ${HEADER_FILES} DESTINATION include/libda)

add_subdirectory(src)
add_subdirectory(plugins)

