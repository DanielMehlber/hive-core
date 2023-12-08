find_package(GTest REQUIRED)

option(INCLUDE_ENCODER_EVALUATION "Evaluates different encoders for image transport" OFF)

add_executable(graphicstests test.cpp
        PeerSetup.h)
target_link_libraries(graphicstests PUBLIC GTest::GTest ${Boost_LIBRARIES} jobsystem logging events networking graphics services)

if (INCLUDE_ENCODER_EVALUATION)
    target_compile_definitions(graphicstests PRIVATE INCLUDE_ENCODER_EVALUATION)
    message("Encoder Evaluation is activated")
else ()
    message("Encoder Evaluation is not activated")
endif ()

add_test(NAME GraphicsTests COMMAND graphicstests)