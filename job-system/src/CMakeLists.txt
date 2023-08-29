add_library(job-system Job.cpp JobManager.cpp SingleThreadedExecutionImpl.cpp)
target_include_directories(job-system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(job-system logging)