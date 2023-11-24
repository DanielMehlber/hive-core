find_package(GTest REQUIRED)

add_executable(commontest test.cpp)
target_link_libraries(commontest PUBLIC GTest::GTest ${Boost_LIBRARIES} common)

add_test(NAME CommonTest COMMAND commontest)