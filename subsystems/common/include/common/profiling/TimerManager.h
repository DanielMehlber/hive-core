#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace common::profiling {

/**
 * Collects measurements of all timers and analyzes them.
 */
class TimerManager {
private:
  std::map<std::string, long> m_measurements;
  static std::shared_ptr<TimerManager> m_global_instance;

public:
  static std::shared_ptr<TimerManager> GetGlobal();
  void Reset();
  std::string Print();
  void Commit(const std::string &name, long ns);
};

} // namespace common::profiling

#endif // TIMERMANAGER_H
