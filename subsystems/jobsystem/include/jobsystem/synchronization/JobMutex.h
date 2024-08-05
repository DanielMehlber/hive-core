#pragma once

#include <condition_variable>
#include <mutex>

#ifdef JOB_SYSTEM_SINGLE_THREAD
namespace hive::jobsystem {
typedef std::mutex mutex;
typedef std::recursive_mutex recursive_mutex;
} // namespace hive::jobsystem
#else
// define synchronization types for fibers
#include "jobsystem/execution/impl/fiber/BoostFiberRecursiveSpinLock.h"
#include "jobsystem/execution/impl/fiber/BoostFiberSpinLock.h"
namespace hive::jobsystem {
typedef BoostFiberSpinLock mutex;
typedef BoostFiberRecursiveSpinLock recursive_mutex;
} // namespace hive::jobsystem
#endif
