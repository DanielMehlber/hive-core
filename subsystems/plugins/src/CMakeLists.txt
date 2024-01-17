add_library(plugins STATIC
        BoostPluginManager.cpp
)

target_include_directories(plugins PUBLIC ../include ${Boost_INCLUDE_DIRS}})
target_link_libraries(plugins PUBLIC ${Boost_LIBRARIES} logging jobsystem events common networking services resourcemgmt scene graphics)