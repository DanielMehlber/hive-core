#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

namespace common::sync {

/**
 * In contrast to the kernel-space std::mutex (causing expensive system calls),
 * this synchronization primitive is user-space and may be more efficient in
 * some scenarios. Furthermore, its usable with threads and fibers (see the job
 * system implementation) alike.
 */
class SpinLock {
protected:
  std::atomic_flag m_atomic;
  bool TryAcquire();

public:
  SpinLock() : m_atomic(false) {}
  void lock();
  void unlock();
};
} // namespace common::sync

#endif // SPINLOCK_H
