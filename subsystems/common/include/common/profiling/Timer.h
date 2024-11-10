#pragma once

#include "common/profiling/TimerManager.h"
#include <chrono>

namespace hive::common::profiling {

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

class Timer {
  std::string m_name;
  time_point m_start;
  bool m_running;

public:
  explicit Timer(std::string name);
  ~Timer();

  void Start();
  void Stop();
};

} // namespace hive::common::profiling
