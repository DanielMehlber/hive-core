add_library(kernel
        Kernel.cpp)

target_include_directories(kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS} ${jobsystem_INCLUDE_DIRS} ${messaging_INCLUDE_DIRS} ${common_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS} ${graphics_INCLUDE_DIRS})
target_link_libraries(kernel PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking scene graphics services resourcemgmt properties plugins)