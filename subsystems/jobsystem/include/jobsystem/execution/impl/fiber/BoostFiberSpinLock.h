#ifndef BOOSTFIBERSPINLOCK_H
#define BOOSTFIBERSPINLOCK_H

#include <atomic>

namespace jobsystem {

/**
 * A user-space mutex alternative which is built to integrate seamlessly with
 * fibers provided by the boost library. Instead of blocking or busy waiting,
 * the current fiber (or thread) yields.
 *
 * @note Normal std::mutex implementations only work on the thread-level and do
 * not recognize fibers, causing various errors on different platforms.
 *
 * @note The Boost.Fiber library also provides their own mutex implementation
 * for the same purpose. However, for some reason, it only works with fibers and
 * cannot handle normal threads (blindly high-jacks their contexts, resulting in
 * deadlocks and undefined behavior).
 */
class BoostFiberSpinLock {
protected:
  std::atomic_flag m_lock;
  void yield() const;

public:
  BoostFiberSpinLock() : m_lock() { m_lock.clear(); }
  void lock();
  void unlock();
  bool try_lock();
};

} // namespace jobsystem

#endif // BOOSTFIBERSPINLOCK_H
