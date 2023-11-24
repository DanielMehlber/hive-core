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

target_include_directories(services PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(services PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking)