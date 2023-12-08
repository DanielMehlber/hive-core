find_package(GTest REQUIRED)

add_executable(eventstests test.cpp)
target_link_libraries(eventstests PUBLIC GTest::GTest ${Boost_LIBRARIES} jobsystem logging events)

add_test(NAME EventsTests COMMAND eventstests)