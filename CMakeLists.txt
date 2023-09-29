cmake_minimum_required(VERSION 3.10)
project(simulation-framework)

enable_testing()

add_subdirectory(common)
add_subdirectory(logging)
add_subdirectory(jobsystem)
add_subdirectory(messaging)
add_subdirectory(resourcemgmt)
add_subdirectory(networking)