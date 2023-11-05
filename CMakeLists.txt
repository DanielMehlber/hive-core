cmake_minimum_required(VERSION 3.10)
project(simulation-framework)

enable_testing()

option(GENERATE_DOCS "Run Doxygen to build the documentation" OFF)

add_subdirectory(common)
add_subdirectory(logging)
add_subdirectory(jobsystem)
add_subdirectory(messaging)
add_subdirectory(properties)
add_subdirectory(resourcemgmt)
add_subdirectory(networking)
add_subdirectory(services)
add_subdirectory(scene)
add_subdirectory(graphics)
add_subdirectory(kernel)