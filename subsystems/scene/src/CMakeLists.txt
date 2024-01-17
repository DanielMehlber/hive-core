add_library(scene STATIC SceneManager.cpp)
target_include_directories(scene PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(scene PUBLIC ${Boost_LIBRARIES} logging vsg::vsg)