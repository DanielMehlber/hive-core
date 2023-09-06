find_package(GTest REQUIRED)

add_executable(eventsystemtests test.cpp)
target_link_libraries(eventsystemtests PUBLIC ${GTest_LIBRARIES} jobsystem logging eventsystem)

add_test(NAME JobTests COMMAND eventsystemtests)