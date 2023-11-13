add_library(graphics
        OffscreenRenderer.cpp
        OnscreenRenderer.cpp
        RenderService.cpp
)

target_include_directories(graphics PUBLIC ../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS} ${logging_INCLUDE_DIRS})
target_link_libraries(graphics PUBLIC ${Boost_LIBRARIES} logging jobsystem common scene vsg::vsg services)