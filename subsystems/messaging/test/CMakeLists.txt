find_package(GTest REQUIRED)

add_executable(messagingtests test.cpp)
target_link_libraries(messagingtests PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging)

add_test(NAME MessagingTests COMMAND messagingtests)