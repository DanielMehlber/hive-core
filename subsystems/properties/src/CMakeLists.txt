add_library(properties PropertyChangeListener.cpp PropertyProvider.cpp
        ../include/properties/Properties.h)
target_include_directories(properties PUBLIC ../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS})
target_link_libraries(properties PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common)