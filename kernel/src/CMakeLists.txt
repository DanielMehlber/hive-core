set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(kernel SHARED
        Kernel.cpp
)

install(TARGETS kernel 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)

target_include_directories(kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(kernel PUBLIC ${Boost_LIBRARIES} logging jobsystem events common networking scene graphics services resourcemgmt properties plugins)