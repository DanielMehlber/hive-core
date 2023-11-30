add_library(networking
        UrlParser.cpp
        PeerMessage.cpp
        PeerMessageConsumerJob.cpp
        PeerMessageConverter.cpp
        NetworkingManager.cpp
        BoostWebSocketPeer.cpp
        BoostWebSocketConnection.cpp
        BoostWebSocketConnectionListener.cpp
        BoostWebSocketConnectionEstablisher.cpp
        MultipartFormdata.cpp)
target_include_directories(networking PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(networking PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common properties)