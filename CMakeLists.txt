cmake_minimum_required(VERSION 3.10)
project(simulation-framework)

enable_testing()

option(GENERATE_DOCS "Run Doxygen to build the documentation" OFF)

# necessary for building shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(subsystems)
add_subdirectory(kernel)
add_subdirectory(standalone)
add_subdirectory(plugins)