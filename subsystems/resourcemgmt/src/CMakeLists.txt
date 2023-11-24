add_library(resourcemgmt ThreadPoolResourceManager.cpp FileLoader.cpp
        ../include/resourcemgmt/ResourceManagement.h)
target_include_directories(resourcemgmt PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(resourcemgmt PUBLIC ${Boost_LIBRARIES} logging jobsystem common)