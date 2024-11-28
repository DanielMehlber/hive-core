find_package(GTest REQUIRED)

add_executable(servicestest test.cpp)
target_link_libraries(servicestest PUBLIC GTest::GTest ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging
        hive-events
        hive-data
        hive-services)

add_test(NAME ServicesTest COMMAND servicestest)