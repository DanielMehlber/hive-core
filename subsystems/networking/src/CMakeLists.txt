add_library(networking STATIC
        UrlParser.cpp
        Message.cpp
        MessageConsumerJob.cpp
        MessageConverter.cpp
        NetworkingManager.cpp
        BoostWebSocketEndpoint.cpp
        BoostWebSocketConnection.cpp
        BoostWebSocketConnectionListener.cpp
        BoostWebSocketConnectionEstablisher.cpp
        MultipartFormdata.cpp
)
target_include_directories(networking PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(networking PUBLIC ${Boost_LIBRARIES} logging jobsystem events common properties)