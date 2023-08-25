cmake_minimum_required(VERSION 3.10)
project(job-system)

find_package(Boost REQUIRED)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)
