cmake_minimum_required(VERSION 3.10)
project(simulation-framework)

enable_testing()

option(GENERATE_DOCS "Run Doxygen to build the documentation" OFF)

# necessary for building shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()

add_subdirectory(subsystems)
add_subdirectory(kernel)
add_subdirectory(plugins)
add_subdirectory(standalone)