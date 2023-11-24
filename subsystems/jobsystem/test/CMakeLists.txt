find_package(GTest REQUIRED)

add_executable(jobsystemtests test.cpp)
target_link_libraries(jobsystemtests PUBLIC GTest::GTest ${Boost_LIBRARIES} jobsystem logging)

add_test(NAME JobTests COMMAND jobsystemtests)