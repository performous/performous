cmake_minimum_required(VERSION 2.6)

# Make plugin++ and libda not install themselves on the system
set(SLAVE_LIB TRUE)

add_subdirectory(plugin++)
add_subdirectory(libda)

