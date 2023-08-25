find_package(GTest REQUIRED)

add_executable(job-system-tests test.cpp)
target_link_libraries(job-system-tests PUBLIC ${GTest_LIBRARIES} job-system logging)

add_test(NAME JobTests COMMAND job-system-tests)