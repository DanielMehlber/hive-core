find_package(GTest REQUIRED)

add_executable(networkingtests test.cpp)
target_link_libraries(networkingtests PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging networking)

add_test(NAME NetworkingTests COMMAND networkingtests)