find_package(GTest REQUIRED)

add_executable(pluginstest test.cpp)
target_link_libraries(pluginstest PUBLIC GTest::GTest ${Boost_LIBRARIES}
        hive-common
        hive-plugins)

add_test(NAME PluginsTest COMMAND pluginstest)