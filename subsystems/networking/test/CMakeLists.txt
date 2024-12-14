find_package(GTest REQUIRED)

add_executable(networkingtests test.cpp)
target_link_libraries(networkingtests PUBLIC GTest::GTest ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging
        hive-events
        hive-networking)

add_test(NAME NetworkingTests COMMAND networkingtests)