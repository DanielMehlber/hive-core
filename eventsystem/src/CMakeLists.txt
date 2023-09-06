add_library(eventsystem JobBasedEventManager.cpp)
target_include_directories(eventsystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS})
target_link_libraries(eventsystem PUBLIC ${Boost_LIBRARIES} logging jobsystem)