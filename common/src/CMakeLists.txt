add_library(common common.cpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(common PUBLIC ${Boost_LIBRARIES})