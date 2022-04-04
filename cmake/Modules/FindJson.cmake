# - Try to find nlohmann_json
# Once done, this will define
#
#  Json_FOUND - system has nlohmann_json
#  Json_INCLUDE_DIRS - the nlohmann_json include directories
#  Json_LIBRARIES - link these to use nlohmann_json
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_detect(Json nlohmann_json FIND_PATH nlohmann/json.hpp)
set(Json_VERSION ${Json_PKGCONF_VERSION})
libfind_process(Json)
