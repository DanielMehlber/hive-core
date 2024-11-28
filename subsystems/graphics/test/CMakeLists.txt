find_package(GTest REQUIRED)

option(INCLUDE_ENCODER_EVALUATION "Evaluates different encoders for image transport" OFF)

add_executable(graphicstests test.cpp
        PeerSetup.h)
target_link_libraries(graphicstests PUBLIC GTest::GTest ${Boost_LIBRARIES}
        hive-common
        hive-jobsystem
        hive-logging
        hive-events
        hive-networking
        hive-graphics
        hive-services)

add_test(NAME GraphicsTests COMMAND graphicstests)