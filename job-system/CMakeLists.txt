cmake_minimum_required(VERSION 3.10)

project(job-system)

find_package(Boost REQUIRED)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)

set_property(TARGET job-system PROPERTY CXX_STANDARD 20)
set_property(TARGET job-system PROPERTY CXX_STANDARD_REQUIRED On)
target_compile_features(job-system PUBLIC cxx_std_20)
