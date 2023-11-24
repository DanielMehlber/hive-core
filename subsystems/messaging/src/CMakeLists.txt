add_library(messaging JobBasedMessageBroker.cpp)
target_include_directories(messaging PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(messaging PUBLIC ${Boost_LIBRARIES} logging jobsystem)