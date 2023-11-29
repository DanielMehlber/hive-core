add_library(networking
        UrlParser.cpp
        PeerMessage.cpp
        PeerMessageConsumerJob.cpp
        BoostJsonMessageConverter.cpp
        NetworkingManager.cpp
        BoostWebSocketPeer.cpp
        BoostWebSocketConnection.cpp
        BoostWebSocketConnectionListener.cpp
        BoostWebSocketConnectionEstablisher.cpp
        ../include/networking/Networking.h)
target_include_directories(networking PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(networking PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common properties)