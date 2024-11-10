#include "jobsystem/execution/impl/fiber/BoostFiberRecursiveSpinLock.h"
#include "common/assert/Assert.h"

using namespace hive::jobsystem;

OwnerInfo GetCurrentOwnerInfo() {
  bool is_called_from_fiber = !boost::fibers::context::active()->is_context(
      boost::fibers::type::main_context);

  auto id = OwnerId{};
  if (is_called_from_fiber) {
    id.fiber_id = boost::this_fiber::get_id();
    return OwnerInfo{true, id};
  } else {
    id.thread_id = std::this_thread::get_id();
    return OwnerInfo{false, id};
  }
}

void BoostFiberRecursiveSpinLock::unlock() {
  std::unique_lock lock(m_owner_info_mutex);

  DEBUG_ASSERT(m_owner_info.has_value(), "should be locked by owner");
  DEBUG_ASSERT(m_owner_info.value() == GetCurrentOwnerInfo(),
               "only current owner is allowed to unlock");
  DEBUG_ASSERT(m_owner_lock_count > 0, "should be locked");
  DEBUG_ASSERT(m_lock.test(), "should be locked");

  if (m_owner_info) {
    auto owner_info = m_owner_info.value();

    if (owner_info == GetCurrentOwnerInfo() && m_owner_lock_count.load() > 1) {
      // this fiber/thread has already acquired this lock, so skip to avoid
      // deadlock
      m_owner_lock_count.fetch_sub(1);
      return;
    }
  }

  m_lock.clear();
  m_owner_info.reset();
  m_owner_lock_count.store(0);
}

void BoostFiberRecursiveSpinLock::lock() {
  while (!try_lock()) {
    /* A fiber can context switch even though it has aquired a lock.
    To avoid deadlocks, other waiting fibers/threads must not block. */
    yield();
  }
}

void BoostFiberRecursiveSpinLock::yield() const {
  auto owner_info = GetCurrentOwnerInfo();
  if (owner_info.is_fiber) {
    boost::this_fiber::yield();
  } else {
    std::this_thread::yield();
  }
}

bool BoostFiberRecursiveSpinLock::try_lock() {

  // first check if this thread/fiber has already aquired this lock
  {
    std::unique_lock lock(m_owner_info_mutex);
    if (m_owner_info) {
      auto &owner_info = m_owner_info.value();

      DEBUG_ASSERT(m_owner_lock_count > 0, "shoud be locked");
      DEBUG_ASSERT(m_lock.test(), "shoud be locked");

      if (owner_info == GetCurrentOwnerInfo()) {
        // this fiber/thread has already acquired this lock, so skip to avoid
        // deadlock
        m_owner_lock_count.fetch_add(1);
        return true;
      }
    }
  }

  // sets the current value to true and returns the value the flag held before
  bool alreadyLocked = m_lock.test_and_set(std::memory_order_acquire);

  // if lock has been aquired, store current owner info
  if (!alreadyLocked) {
    std::unique_lock lock(m_owner_info_mutex);
    m_owner_info = GetCurrentOwnerInfo();
    m_owner_lock_count.store(1);
  }

  return !alreadyLocked;
}
