find_package(GTest REQUIRED)

add_executable(jobsystemtests test.cpp)
target_link_libraries(jobsystemtests PUBLIC GTest::GTest
        ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging)

add_test(NAME JobTests COMMAND jobsystemtests)