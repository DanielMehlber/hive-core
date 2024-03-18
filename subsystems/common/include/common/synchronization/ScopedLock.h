#ifndef SCOPEDLOCK_H
#define SCOPEDLOCK_H

namespace common::sync {

/**
 * Equivalent to std::unique_lock for the spin-lock mutex alternative.
 * @tparam Lock lock type (e.g. spin lock)
 */
template <class Lock> class ScopedLock {
  typedef Lock lock_t;
  lock_t *m_pLock;

public:
  explicit ScopedLock(lock_t &lock) : m_pLock(&lock) { m_pLock->lock(); }
  ScopedLock(ScopedLock &) = delete;
  ~ScopedLock() { m_pLock->unlock(); }
};

} // namespace common::sync

#endif // SCOPEDLOCK_H
