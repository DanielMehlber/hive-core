#ifndef JOBMUTEX_H
#define JOBMUTEX_H

#include <condition_variable>
#include <mutex>

#ifdef JOB_SYSTEM_SINGLE_THREAD
namespace jobsystem {
typedef std::mutex mutex;
typedef std::condition_variable condition_variable;
typedef std::recursive_mutex recursive_mutex;
} // namespace jobsystem
#else
// define synchronization types for fibers
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/recursive_mutex.hpp>
namespace jobsystem {
typedef boost::fibers::mutex mutex;
typedef boost::fibers::condition_variable condition_variable;
typedef boost::fibers::recursive_mutex recursive_mutex;
} // namespace jobsystem
#endif

#endif // JOBMUTEX_H
