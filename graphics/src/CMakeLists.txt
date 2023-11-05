add_library(graphics
        OffscreenRenderer.cpp
        OnscreenRenderer.cpp
)

target_include_directories(graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS} ${logging_INCLUDE_DIRS})
target_link_libraries(graphics PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking scene vsg::vsg)