add_library(networking
        UrlParser.cpp
        WebSocketMessage.cpp
        WebSocketMessageConsumerJob.cpp
        BoostJsonMessageConverter.cpp
        NetworkingManager.cpp
        BoostWebSocketPeer.cpp
        BoostWebSocketConnection.cpp
        BoostWebSocketConnectionListener.cpp
        BoostWebSocketConnectionEstablisher.cpp
        ../include/networking/Networking.h)
target_include_directories(networking PUBLIC ../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${properties_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS})
target_link_libraries(networking PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common properties)