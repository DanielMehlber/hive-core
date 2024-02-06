#include "common/profiling/TimerManager.h"
#include <numeric>

using namespace common::profiling;

std::shared_ptr<TimerManager> TimerManager::m_global_instance = nullptr;

std::shared_ptr<TimerManager> TimerManager::GetGlobal() {
  if (!m_global_instance) {
    m_global_instance = std::make_shared<TimerManager>();
  }
  return m_global_instance;
}

void TimerManager::Reset() { m_measurements.clear(); };

float ns_to_ms(long ns) { return (float)ns / 1000000; }

std::string TimerManager::Print() {
#ifdef ENABLE_PROFILING
  std::stringstream ss;
  for (auto &[name, data] : m_measurements) {
    long average_ns = std::reduce(data.values.begin(), data.values.end()) /
                      data.values.size();
    long min_ns = *std::min_element(data.values.begin(), data.values.end());
    long max_ns = *std::max_element(data.values.begin(), data.values.end());

    ss << "> Timer " << name << ": TOTAL-MIN " << ns_to_ms(data.total_min_ns)
       << "ms | TOTAL-MAX " << ns_to_ms(data.total_max_ns) << "ms | AVG "
       << ns_to_ms(average_ns) << "ms | MIN " << ns_to_ms(min_ns) << "ms | MAX "
       << ns_to_ms(max_ns) << "ms"
       << "\n";
  }

  return ss.str();
#else
  return "";
#endif
};

void TimerManager::Commit(const std::string &name, long ns) {
#ifdef ENABLE_PROFILING
  auto current = m_measurements.contains(name)
                     ? m_measurements.at(name)
                     : Measurement{LONG_MAX, LONG_MIN, std::vector<long>()};

  current.total_min_ns = std::min(ns, current.total_min_ns);
  current.total_max_ns = std::max(ns, current.total_max_ns);
  current.values.insert(current.values.begin(), ns);
  if (current.values.size() > 10) {
    current.values.resize(10);
  }

  m_measurements[name] = current;
#endif
}