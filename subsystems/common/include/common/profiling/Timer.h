#ifndef TIMER_H
#define TIMER_H

#include "common/profiling/TimerManager.h"
#include <chrono>
#include <functional>

namespace common::profiling {

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

class Timer {
private:
  std::string m_name;
  time_point m_start;
  bool m_running;

public:
  explicit Timer(std::string name);
  ~Timer();

  void Start();
  void Stop();
};

} // namespace common::profiling

#endif // TIMER_H
