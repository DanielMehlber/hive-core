add_library(logging logging/logger/impl/BoostLogger.cpp logging/Logging.cpp)
target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(logging PUBLIC ${Boost_LIBRARIES})