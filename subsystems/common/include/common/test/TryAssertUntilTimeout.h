#pragma once

#include <future>
#include <gtest/gtest.h>
using namespace std::chrono_literals;

namespace common::test {

/**
 * Tries to repeatedly assert a condition until it is true, but only for a
 * limited period of time. After that it fails.
 * @param evaluation condition that will be asserted.
 * @param timeout period of time for assertion. After is has expired and the
 * assertion was not successful, it fails.
 */
inline void TryAssertUntilTimeout(const std::function<bool()> &evaluation,
                                  std::chrono::seconds timeout) {
  auto time = std::chrono::high_resolution_clock::now();
  while (!evaluation()) {
    auto now = std::chrono::high_resolution_clock::now();
    auto delay =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - time);
    if (delay > timeout) {
      FAIL() << "more than " << timeout.count()
             << " seconds passed and condition was still not true";
    }
    std::this_thread::sleep_for(1ms);
  }
}

/**
 * Waits for a future to be ready, but only for a limited period of time. After
 * that the assertion fails.
 * @param future future that will be waited for.
 * @param timeout period of time for waiting. After is has expired and the
 * future was not ready, it fails.
 */
template <typename T>
void WaitForFutureUntilTimeout(const std::future<T> &future,
                               std::chrono::seconds timeout) {
  TryAssertUntilTimeout(
      [&future] { return future.wait_for(0s) == std::future_status::ready; },
      timeout);
}
} // namespace common::test
