find_package(GTest REQUIRED)

add_executable(datatest test.cpp)
target_link_libraries(datatest PUBLIC GTest::GTest ${Boost_LIBRARIES} common jobsystem logging events data data)

add_test(NAME DataLayerTest COMMAND datatest)