set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(core SHARED
        Core.cpp
)

install(TARGETS core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(core PUBLIC ${Boost_LIBRARIES} logging jobsystem events common networking scene graphics services resourcemgmt properties plugins)