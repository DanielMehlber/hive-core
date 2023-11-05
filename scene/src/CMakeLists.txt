add_library(scene
        SceneManager.cpp
)
target_include_directories(scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include ${Boost_INCLUDE_DIRS})
target_link_libraries(scene PUBLIC ${Boost_LIBRARIES} logging vsg::vsg)