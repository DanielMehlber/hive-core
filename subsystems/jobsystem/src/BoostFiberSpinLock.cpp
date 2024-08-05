#include "jobsystem/execution/impl/fiber/BoostFiberSpinLock.h"
#include "common/assert/Assert.h"
#include <boost/fiber/all.hpp>

using namespace hive::jobsystem;

bool isFiber() {
  return !boost::fibers::context::active()->is_context(
      boost::fibers::type::main_context);
}

bool BoostFiberSpinLock::try_lock() {
  // use an acquire fence to ensure all subsequent
  // reads by this thread will be valid
  bool alreadyLocked = m_lock.test_and_set(std::memory_order_acquire);
  return !alreadyLocked;
}

void BoostFiberSpinLock::lock() {
  // spin until successful acquire
  while (!try_lock()) {
    yield();
  }
}

void BoostFiberSpinLock::unlock() {
  DEBUG_ASSERT(m_lock.test(), "cannot unlock unaquired lock");
  // use release semantics to ensure that all prior
  // writes have been fully committed before we unlock
  m_lock.clear(std::memory_order_release);
}

void BoostFiberSpinLock::yield() const {
  if (isFiber()) {
    boost::this_fiber::yield();
  } else {
    std::this_thread::yield();
  }
}