find_package(GTest REQUIRED)

add_executable(graphicstests test.cpp)
target_link_libraries(graphicstests PUBLIC ${GTest_LIBRARIES} jobsystem logging messaging networking graphics services)

add_test(NAME GraphicsTests COMMAND graphicstests)