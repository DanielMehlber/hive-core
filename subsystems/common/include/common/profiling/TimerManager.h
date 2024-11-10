#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hive::common::profiling {

struct Measurement {
  long total_min_ns;
  long total_max_ns;
  std::vector<long> values;
};

/**
 * Collects measurements of all timers and analyzes them.
 */
class TimerManager {
  std::map<std::string, Measurement> m_measurements;
  static std::shared_ptr<TimerManager> m_global_instance;

public:
  static std::shared_ptr<TimerManager> GetGlobal();
  void Reset();
  std::string Print();
  void Commit(const std::string &name, long ns);
};

} // namespace hive::common::profiling
