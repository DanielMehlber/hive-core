find_package(GTest REQUIRED)

add_executable(resourcemgmttest test.cpp)
target_link_libraries(resourcemgmttest PUBLIC ${GTest_LIBRARIES} resourcemgmt jobsystem logging)

add_test(NAME ResourceManagerTest COMMAND resourcemgmttest)