find_package(GTest REQUIRED)

option(INCLUDE_ENCODER_EVALUATION "Evaluates different encoders for image transport" OFF)

add_executable(graphicstests test.cpp
        PeerSetup.h)
target_link_libraries(graphicstests PUBLIC GTest::GTest ${Boost_LIBRARIES}
        common
        jobsystem
        logging
        events
        networking
        graphics
        services)

add_test(NAME GraphicsTests COMMAND graphicstests)