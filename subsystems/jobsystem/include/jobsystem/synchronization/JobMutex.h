#ifndef JOBMUTEX_H
#define JOBMUTEX_H

#include <condition_variable>
#include <mutex>

#ifdef JOB_SYSTEM_SINGLE_THREAD
namespace jobsystem {
typedef std::mutex mutex;
typedef std::condition_variable condition_variable;
} // namespace jobsystem
#else
// define synchronization types for fibers
#include <boost/fiber/mutex.hpp>
namespace jobsystem {
typedef boost::fibers::mutex mutex;
typedef boost::fibers::condition_variable condition_variable;
} // namespace jobsystem
#endif

#endif // SIMULATION_FRAMEWORK_JOBMUTEX_H
