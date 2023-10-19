find_package(GTest REQUIRED)

add_executable(servicestest test.cpp)
target_link_libraries(servicestest PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging properties services)

add_test(NAME PropertiesTest COMMAND servicestest)