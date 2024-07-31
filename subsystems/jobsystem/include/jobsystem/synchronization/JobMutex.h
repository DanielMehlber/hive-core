#pragma once

#include <condition_variable>
#include <mutex>

#ifdef JOB_SYSTEM_SINGLE_THREAD
namespace jobsystem {
typedef std::mutex mutex;
typedef std::recursive_mutex recursive_mutex;
} // namespace jobsystem
#else
// define synchronization types for fibers
#include "jobsystem/execution/impl/fiber/BoostFiberRecursiveSpinLock.h"
#include "jobsystem/execution/impl/fiber/BoostFiberSpinLock.h"
namespace jobsystem {
typedef BoostFiberSpinLock mutex;
typedef BoostFiberRecursiveSpinLock recursive_mutex;
} // namespace jobsystem
#endif
