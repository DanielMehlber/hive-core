#pragma once

#include "boost/fiber/all.hpp"
#include "common/synchronization/SpinLock.h"

namespace hive::jobsystem {

union OwnerId {
  boost::fibers::fiber::id fiber_id;
  std::thread::id thread_id;
};

/**
 * Information about the owner of the recursive spin lock, which is either a
 * thread or a fiber.
 */
struct OwnerInfo {
  bool is_fiber;
  OwnerId id;

  bool operator==(OwnerInfo &other) const {
    if (is_fiber == other.is_fiber) {
      if (is_fiber) {
        return id.fiber_id == other.id.fiber_id;
      } else {
        return id.thread_id == other.id.thread_id;
      }
    }

    return false;
  }
};

/**
 * A user-space recursive mutex alternative which is built to integrate
 * seamlessly with fibers provided by the boost library. Instead of blocking or
 * busy waiting, the current fiber (or thread) yields.
 *
 * @note Normal std::recursive_mutex implementations only work on the
 * thread-level and do not recognize fibers, causing various errors on different
 * platforms.
 *
 * @note The Boost.Fiber library also provides their own recursive_mutex
 * implementation for the same purpose. However, for some reason, it only works
 * with fibers and cannot handle normal threads (blindly high-jacks their
 * contexts, resulting in deadlocks and undefined behavior).
 */
class BoostFiberRecursiveSpinLock {
protected:
  /** another normal spin lock synchronizing access to owner information */
  mutable common::sync::SpinLock m_owner_info_mutex;

  /** information about the current owner (thread or fiber) */
  std::optional<OwnerInfo> m_owner_info;

  /** amount of recursive locks by the current owner */
  std::atomic_int m_owner_lock_count{0};

  /** if true, this is currently locked */
  std::atomic_flag m_lock;

  void yield() const;

public:
  void lock();
  void unlock();
  bool try_lock();
};

} // namespace hive::jobsystem
