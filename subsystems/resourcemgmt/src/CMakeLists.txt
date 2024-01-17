add_library(resourcemgmt STATIC ThreadPoolResourceManager.cpp FileLoader.cpp)
target_include_directories(resourcemgmt PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(resourcemgmt PUBLIC ${Boost_LIBRARIES} logging jobsystem common)