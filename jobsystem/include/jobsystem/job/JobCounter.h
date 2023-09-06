#ifndef JOBCOUNTER_H
#define JOBCOUNTER_H

#include <atomic>
#include <mutex>

namespace jobsystem::job {
class JobCounter {
private:
  std::atomic<size_t> m_count{0};

public:
  void Increase();
  void Decrease();
  bool AreAllFinished();
};

inline void JobCounter::Increase() { m_count++; }
inline void JobCounter::Decrease() { m_count--; }
inline bool JobCounter::AreAllFinished() { return m_count < 1; }

} // namespace jobsystem::job

#endif /* JOBCOUNTER_H */
