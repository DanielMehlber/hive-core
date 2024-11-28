find_package(GTest REQUIRED)

add_executable(eventstests test.cpp)
target_link_libraries(eventstests PUBLIC GTest::GTest
        ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging
        hive-events)

add_test(NAME EventsTests COMMAND eventstests)