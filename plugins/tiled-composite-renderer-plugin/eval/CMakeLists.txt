find_package(GTest REQUIRED)

add_executable(tiled-rendering-plugin-eval eval.cpp)
target_link_libraries(tiled-rendering-plugin-eval PUBLIC kernel)