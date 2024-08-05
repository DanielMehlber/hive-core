#pragma once

#include <atomic>

namespace hive::common::sync {

/**
 * In contrast to the kernel-space std::mutex (causing expensive system calls),
 * this synchronization primitive is user-space and may be more efficient in
 * some scenarios. Furthermore, its usable with threads and fibers (see the job
 * system implementation) alike.
 */
class SpinLock {
protected:
  std::atomic_flag m_lock;

public:
  SpinLock() : m_lock() { m_lock.clear(); }
  void lock();
  void unlock();
  bool try_lock();
};
} // namespace hive::common::sync
