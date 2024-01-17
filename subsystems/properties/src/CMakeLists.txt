add_library(properties STATIC PropertyChangeListener.cpp PropertyProvider.cpp)
target_include_directories(properties PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(properties PUBLIC ${Boost_LIBRARIES} logging jobsystem events common)