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

  m_spin_lock.unlock();
  m_owner_info.reset();
}

void BoostFiberRecursiveSpinLock::lock() {
  // first check if lock has already been acquired by current thread/fiber.
  {
    common::sync::ScopedLock lock(m_owner_info_mutex);
    if (m_owner_info) {
      auto owner_info = m_owner_info.value();

      if (owner_info == GetCurrentOwnerInfo()) {
        // this fiber/thread has already acquired this lock, so skip to avoid
        // deadlock
        m_owner_lock_count.fetch_add(1);
        return;
      }
    }
  }

  m_spin_lock.lock();
  common::sync::ScopedLock lock(m_owner_info_mutex);
  m_owner_info = GetCurrentOwnerInfo();
  m_owner_lock_count.store(1);
}
