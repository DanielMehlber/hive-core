find_package(GTest REQUIRED)

add_executable(pluginstest test.cpp)
target_link_libraries(pluginstest PUBLIC ${GTest_LIBRARIES} plugins)

add_test(NAME PluginsTest COMMAND pluginstest)