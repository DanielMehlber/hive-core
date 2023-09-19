#ifndef JOBCOUNTER_H
#define JOBCOUNTER_H

#include "IJobWaitable.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace jobsystem::job {
class JobCounter : public IJobWaitable {
private:
  std::atomic<size_t> m_count{0};

public:
  void Increase();
  void Decrease();
  bool IsFinished() override;
};

inline void JobCounter::Increase() { m_count++; }
inline void JobCounter::Decrease() { m_count--; }
inline bool JobCounter::IsFinished() { return m_count < 1; }

typedef std::shared_ptr<jobsystem::job::JobCounter> SharedJobCounter;

} // namespace jobsystem::job

#endif /* JOBCOUNTER_H */
