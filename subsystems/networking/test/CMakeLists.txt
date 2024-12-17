find_package(GTest REQUIRED)

add_executable(networkingtests test.cpp)
target_link_libraries(networkingtests PUBLIC GTest::GTest ${Boost_LIBRARIES}
        common
        jobsystem
        logging
        events
        networking)

add_test(NAME NetworkingTests COMMAND networkingtests)