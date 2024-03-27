#include "jobsystem/execution/impl/fiber/BoostFiberRecursiveSpinLock.h"
#include "common/synchronization/ScopedLock.h"

using namespace jobsystem;

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

  common::sync::ScopedLock lock(m_owner_info_mutex);
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
}

void BoostFiberRecursiveSpinLock::lock() {
  while (!try_lock()) {
      // TODO: introduce some sort of yield of sleep to avoid busy waiting
  }
}

bool BoostFiberRecursiveSpinLock::try_lock() {

  // first check if this thread/fiber has already aquired this lock
  {
    common::sync::ScopedLock lock(m_owner_info_mutex);
    if (m_owner_info) {
      auto owner_info = m_owner_info.value();

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
    common::sync::ScopedLock lock(m_owner_info_mutex);
    m_owner_info = GetCurrentOwnerInfo();
    m_owner_lock_count.store(1);
  }

  return !alreadyLocked;
}
