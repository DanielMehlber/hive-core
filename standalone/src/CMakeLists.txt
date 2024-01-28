add_executable(standalone
        main.cpp)

install(TARGETS standalone
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
)

target_include_directories(standalone PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(standalone PUBLIC ${Boost_LIBRARIES} core)