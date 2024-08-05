#include "common/synchronization/SpinLock.h"

using namespace hive::common::sync;

bool SpinLock::try_lock() {
  // use an acquire fence to ensure all subsequent
  // reads by this thread will be valid
  bool alreadyLocked = m_lock.test_and_set(std::memory_order_acquire);
  return !alreadyLocked;
}

void SpinLock::lock() {
  // spin until successful acquire
  while (!try_lock()) {
    // TODO: introduce some sort of yield to avoid busy waiting
  }
}

void SpinLock::unlock() {
  // use release semantics to ensure that all prior
  // writes have been fully committed before we unlock
  m_lock.clear(std::memory_order_release);
}