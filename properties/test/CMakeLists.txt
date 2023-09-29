find_package(GTest REQUIRED)

add_executable(propertiestest test.cpp)
target_link_libraries(propertiestest PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging properties)

add_test(NAME NetworkingTests COMMAND propertiestest)