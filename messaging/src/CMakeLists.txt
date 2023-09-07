add_library(messaging JobBasedMessageBroker.cpp)
target_include_directories(messaging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS})
target_link_libraries(messaging PUBLIC ${Boost_LIBRARIES} logging jobsystem)