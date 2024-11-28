find_package(GTest REQUIRED)

add_executable(datatest test.cpp)
target_link_libraries(datatest PUBLIC GTest::GTest
        ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging
        hive-events
        hive-data)

add_test(NAME DataLayerTest COMMAND datatest)