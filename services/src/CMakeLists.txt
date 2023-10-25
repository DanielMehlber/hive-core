add_library(services
        ServiceRequest.cpp
        ServiceResponse.cpp
        LocalServiceExecutor.cpp
        RoundRobinServiceCaller.cpp
        local-only/LocalOnlyServiceRegistry.cpp
        websockets/WebSocketServiceRegistrationConsumer.cpp
        websockets/WebSocketServiceExecutor.cpp
        websockets/WebSocketServiceResponseConsumer.cpp
        websockets/WebSocketServiceMessagesConverter.cpp
        websockets/WebSocketServiceRegistry.cpp
        websockets/WebSocketServiceRequestConsumer.cpp)

target_include_directories(services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS})
target_link_libraries(services PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking)