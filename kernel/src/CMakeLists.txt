add_library(kernel SHARED
        Kernel.cpp
)


target_include_directories(kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(kernel PUBLIC ${Boost_LIBRARIES} logging jobsystem messaging common networking scene graphics services resourcemgmt properties plugins)