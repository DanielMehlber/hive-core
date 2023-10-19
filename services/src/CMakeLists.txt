add_library(services
        ServiceStub.cpp
        ServiceRequest.cpp
        ServiceResponse.cpp
        LocalServiceRegistry.cpp
        LocalServiceStub.cpp)
target_include_directories(services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS})
target_link_libraries(services PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking)