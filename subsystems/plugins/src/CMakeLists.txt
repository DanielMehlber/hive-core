add_library(plugins
        BoostPluginManager.cpp
)

target_include_directories(plugins PUBLIC ../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS} ${logging_INCLUDE_DIRS})
target_link_libraries(plugins PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking services resourcemgmt scene graphics)