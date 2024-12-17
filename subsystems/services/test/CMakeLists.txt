find_package(GTest REQUIRED)

add_executable(servicestest test.cpp)
target_link_libraries(servicestest PUBLIC GTest::GTest ${Boost_LIBRARIES}
        common
        jobsystem
        logging
        events
        data
        services)

add_test(NAME ServicesTest COMMAND servicestest)