add_library(jobsystem Job.cpp JobManager.cpp SingleThreadedExecutionImpl.cpp TimerJob.cpp FiberExecutionImpl.cpp)
target_include_directories(jobsystem PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(jobsystem logging common ${Boost_Libraries})