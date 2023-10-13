add_library(resourcemgmt ThreadPoolResourceManager.cpp FileLoader.cpp
        ../include/resourcemgmt/ResourceManagement.h)
target_include_directories(resourcemgmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${logging_INCLUDE_DIRS} ${common_INCLUDE_DIRS})
target_link_libraries(resourcemgmt PUBLIC ${Boost_LIBRARIES} logging jobsystem common)