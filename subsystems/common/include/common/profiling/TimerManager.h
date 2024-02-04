#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace common::profiling {

class TimerManager {
private:
  std::map<std::string, long> m_measurements;

public:
  static TimerManager &Get() {
    static TimerManager manager;
    return manager;
  }

  inline void Reset() { m_measurements.clear(); };

  inline std::string Print() {
#ifdef ENABLE_PROFILING
    std::stringstream ss;
    for (auto &[name, ns] : m_measurements) {
      ss << "> Timer " << name << ": " << ns << "ns (" << (float)ns / 1000000
         << "ms)"
         << "\n";
    }

    return ss.str();
#else
    return "";
#endif
  };

  inline void Commit(const std::string &name, long ns) {
#ifdef ENABLE_PROFILING
    auto value = m_measurements.contains(name) ? m_measurements.at(name) : ns;
    m_measurements[name] = (value + ns) / 2;
#endif
  }
};

} // namespace common::profiling

#endif // TIMERMANAGER_H
