#include <utility>

#include "common/profiling/Timer.h"

using namespace common::profiling;

Timer::Timer(std::string name) : m_name(std::move(name)), m_running(false) {
  Start();
}

Timer::~Timer() { Stop(); }

void Timer::Start() {
  m_start = std::chrono::high_resolution_clock::now();
  m_running = true;
}

void Timer::Stop() {
  if (!m_running)
    return;

  // measure elapsed time
  time_point now = std::chrono::high_resolution_clock::now();
  auto elapsed_time = now - m_start;

  long elapsed_time_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time)
          .count();

  TimerManager::GetGlobal()->Commit(m_name, elapsed_time_ns);
  m_running = false;
}
