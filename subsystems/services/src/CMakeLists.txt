add_library(services STATIC
        ServiceRequest.cpp
        ServiceResponse.cpp
        LocalServiceExecutor.cpp
        RoundRobinServiceCaller.cpp
        local-only/LocalOnlyServiceRegistry.cpp
        remote/RemoteServiceRegistrationConsumer.cpp
        remote/RemoteServiceExecutor.cpp
        remote/RemoteServiceResponseConsumer.cpp
        remote/RemoteServiceMessagesConverter.cpp
        remote/RemoteServiceRegistry.cpp
        remote/RemoteServiceRequestConsumer.cpp)

target_include_directories(services PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(services PUBLIC ${Boost_LIBRARIES} logging jobsystem events common networking)