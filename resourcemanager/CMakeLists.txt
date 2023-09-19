cmake_minimum_required(VERSION 3.10)
project(resourcemgmt)

find_package(Boost REQUIRED)

add_subdirectory(src)
add_subdirectory(test)