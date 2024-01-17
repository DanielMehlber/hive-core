add_library(events STATIC JobBasedEventBroker.cpp)
target_include_directories(events PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(events PUBLIC ${Boost_LIBRARIES} logging jobsystem)