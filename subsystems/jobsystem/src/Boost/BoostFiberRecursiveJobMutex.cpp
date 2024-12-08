#include "common/assert/Assert.h"
#include "common/synchronization/SpinLock.h"
#include "jobsystem/synchronization/JobRecursiveMutex.h"

#include <boost/fiber/all.hpp>

using namespace hive::jobsystem;

/** Owner can either be a thread or a fiber */
union OwnerId {
  boost::fibers::fiber::id fiber_id;
  std::thread::id thread_id;
};

/**
 * Information about the owner of the recursive spin lock, which is either a
 * thread or a fiber. This is required to pause and resume the execution of
 * either the current thread or fiber (which are different operations).
 */
struct OwnerInfo {
  bool is_fiber;
  OwnerId id;

  bool operator==(const OwnerInfo &other) const {
    if (is_fiber == other.is_fiber) {
      if (is_fiber) {
        return id.fiber_id == other.id.fiber_id;
      }
      return id.thread_id == other.id.thread_id;
    }

    return false;
  }
};

struct JobRecursiveMutex::Impl {
  /** another normal spin lock synchronizing access to owner information */
  mutable common::sync::SpinLock owner_info_mutex;

  /** information about the current owner (thread or fiber) */
  std::optional<OwnerInfo> owner_info;

  /** amount of recursive locks by the current owner */
  std::atomic_int owner_lock_count{0};

  /** if true, this is currently locked */
  std::atomic_flag lock;
};

OwnerInfo GetCurrentOwnerInfo() {
  bool is_called_from_fiber = !boost::fibers::context::active()->is_context(
      boost::fibers::type::main_context);

  auto id = OwnerId{};
  if (is_called_from_fiber) {
    id.fiber_id = boost::this_fiber::get_id();
    return OwnerInfo{true, id};
  }

  id.thread_id = std::this_thread::get_id();
  return OwnerInfo{false, id};
}

JobRecursiveMutex::JobRecursiveMutex() : m_impl(std::make_unique<Impl>()) {}
JobRecursiveMutex::~JobRecursiveMutex() = default;
JobRecursiveMutex::JobRecursiveMutex(JobRecursiveMutex &&other) noexcept =
    default;
JobRecursiveMutex &
JobRecursiveMutex::operator=(JobRecursiveMutex &&other) noexcept = default;

void JobRecursiveMutex::unlock() {
  std::unique_lock lock(m_impl->owner_info_mutex);

  DEBUG_ASSERT(m_impl->owner_info.has_value(), "should be locked by owner");
  DEBUG_ASSERT(m_impl->owner_info.value() == GetCurrentOwnerInfo(),
               "only current owner is allowed to unlock");
  DEBUG_ASSERT(m_impl->owner_lock_count > 0, "should be locked");
  DEBUG_ASSERT(m_impl->lock.test(), "should be locked");

  if (m_impl->owner_info) {
    const auto owner_info = m_impl->owner_info.value();

    if (owner_info == GetCurrentOwnerInfo() &&
        m_impl->owner_lock_count.load() > 1) {
      // this fiber/thread has already acquired this lock, so skip to avoid
      // deadlock
      m_impl->owner_lock_count.fetch_sub(1);
      return;
    }
  }

  m_impl->lock.clear();
  m_impl->owner_info.reset();
  m_impl->owner_lock_count.store(0);
}

void yield() {
  const auto [is_fiber, id] = GetCurrentOwnerInfo();
  if (is_fiber) {
    boost::this_fiber::yield();
  } else {
    std::this_thread::yield();
  }
}

void JobRecursiveMutex::lock() {
  while (!try_lock()) {
    /* A fiber can context switch even though it has aquired a lock.
    To avoid deadlocks, other waiting fibers/threads must not block. */
    yield();
  }
}

bool JobRecursiveMutex::try_lock() {

  // first check if this thread/fiber has already aquired this lock
  {
    std::unique_lock lock(m_impl->owner_info_mutex);
    if (m_impl->owner_info) {
      auto &owner_info = m_impl->owner_info.value();

      DEBUG_ASSERT(m_impl->owner_lock_count > 0, "shoud be locked");
      DEBUG_ASSERT(m_impl->lock.test(), "shoud be locked");

      if (owner_info == GetCurrentOwnerInfo()) {
        // this fiber/thread has already acquired this lock, so skip to avoid
        // deadlock
        m_impl->owner_lock_count.fetch_add(1);
        return true;
      }
    }
  }

  // sets the current value to true and returns the value the flag held before
  const bool alreadyLocked =
      m_impl->lock.test_and_set(std::memory_order_acquire);

  // if lock has been aquired, store current owner info
  if (!alreadyLocked) {
    std::unique_lock lock(m_impl->owner_info_mutex);
    m_impl->owner_info = GetCurrentOwnerInfo();
    m_impl->owner_lock_count.store(1);
  }

  return !alreadyLocked;
}
