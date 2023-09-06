find_package(GTest REQUIRED)

add_executable(eventsystemtests test.cpp)
target_link_libraries(eventsystemtests PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging)

add_test(NAME EventTests COMMAND eventsystemtests)