cmake_minimum_required(VERSION 3.28)
project(hive_demo_plugin)

set(CMAKE_CXX_STANDARD 20)

# pull entire core framework instead of individual components
find_package(Hive REQUIRED COMPONENTS core)

# build sources
add_library(hive_demo_plugin SHARED
        src/DemoPlugin.cpp
)

target_include_directories(hive_demo_plugin PUBLIC
        include
)

target_link_libraries(hive_demo_plugin
        Hive::core
)