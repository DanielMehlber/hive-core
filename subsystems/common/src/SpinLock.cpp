#include "common/synchronization/SpinLock.h"

using namespace common::sync;

bool SpinLock::TryAcquire() {
  // use an acquire fence to ensure all subsequent
  // reads by this thread will be valid
  bool alreadyLocked = m_atomic.test_and_set(std::memory_order_acquire);
  return !alreadyLocked;
}

void SpinLock::Acquire() {
  // spin until successful acquire
  while (!TryAcquire()) {
    // TODO: introduce some sort of yield to avoid busy waiting
  }
}

void SpinLock::Release() {
  // use release semantics to ensure that all prior
  // writes have been fully committed before we unlock
  m_atomic.clear(std::memory_order_release);
}