#pragma once

#include <memory>

namespace hive::jobsystem {

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
class JobRecursiveMutex {

  /**
   * Pointer to implementation (Pimpl) in source file. This is necessary to
   * constrain the underlying implementation's details in the source file and
   * not expose them to the rest of the application.
   * @note Indispensable for ABI stability.
   */
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  JobRecursiveMutex();
  ~JobRecursiveMutex();

  JobRecursiveMutex(const JobRecursiveMutex &other) = delete;
  JobRecursiveMutex &operator=(const JobRecursiveMutex &other) = delete;

  JobRecursiveMutex(JobRecursiveMutex &&other) noexcept;
  JobRecursiveMutex &operator=(JobRecursiveMutex &&other) noexcept;

  void lock();
  void unlock();
  bool try_lock();
};

typedef JobRecursiveMutex recursive_mutex;

} // namespace hive::jobsystem
