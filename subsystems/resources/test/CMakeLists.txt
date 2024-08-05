find_package(GTest REQUIRED)

add_executable(resourcestest test.cpp)
target_link_libraries(resourcestest PUBLIC GTest::GTest ${Boost_LIBRARIES} resources jobsystem logging)

add_test(NAME ResourceManagerTest COMMAND resourcestest)