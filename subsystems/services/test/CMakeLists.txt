find_package(GTest REQUIRED)

add_executable(servicestest test.cpp)
target_link_libraries(servicestest PUBLIC GTest::GTest ${Boost_LIBRARIES} jobsystem logging events properties services)

add_test(NAME ServicesTest COMMAND servicestest)