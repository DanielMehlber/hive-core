#ifndef TRYASSERTUNTILTIMEOUT_H
#define TRYASSERTUNTILTIMEOUT_H

#include <gtest/gtest.h>
using namespace std::chrono_literals;

namespace common::test {
void TryAssertUntilTimeout(std::function<bool()> evaluation,
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
    std::this_thread::sleep_for(0.05s);
  }
}
} // namespace common::test
#endif // TRYASSERTUNTILTIMEOUT_H
