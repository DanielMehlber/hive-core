#pragma once

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace common::profiling {

struct Measurement {
  long total_min_ns;
  long total_max_ns;
  std::vector<long> values;
};

/**
 * Collects measurements of all timers and analyzes them.
 */
class TimerManager {
private:
  std::map<std::string, Measurement> m_measurements;
  static std::shared_ptr<TimerManager> m_global_instance;

public:
  static std::shared_ptr<TimerManager> GetGlobal();
  void Reset();
  std::string Print();
  void Commit(const std::string &name, long ns);
};

} // namespace common::profiling
