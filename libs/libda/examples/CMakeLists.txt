project("Libda examples" CXX)
cmake_minimum_required(VERSION 2.4)

add_executable(da_capture capture.cpp)
target_link_libraries(da_capture da)
install(TARGETS da_capture DESTINATION bin)

