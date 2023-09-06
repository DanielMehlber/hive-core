add_library(jobsystem Job.cpp JobManager.cpp SingleThreadedExecutionImpl.cpp TimerJob.cpp FiberExecutionImpl.cpp)
target_include_directories(jobsystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(jobsystem logging)