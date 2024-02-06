#include "common/profiling/TimerManager.h"

using namespace common::profiling;

std::shared_ptr<TimerManager> TimerManager::m_global_instance = nullptr;

std::shared_ptr<TimerManager> TimerManager::GetGlobal() {
  if (!m_global_instance) {
    m_global_instance = std::make_shared<TimerManager>();
  }
  return m_global_instance;
}

void TimerManager::Reset() { m_measurements.clear(); };

std::string TimerManager::Print() {
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

void TimerManager::Commit(const std::string &name, long ns) {
#ifdef ENABLE_PROFILING
  auto value = m_measurements.contains(name) ? m_measurements.at(name) : ns;
  m_measurements[name] = (value + ns) / 2;
#endif
}