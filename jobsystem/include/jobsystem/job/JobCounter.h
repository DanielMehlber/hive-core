#ifndef JOBCOUNTER_H
#define JOBCOUNTER_H

#include "IJobWaitable.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace jobsystem::job {

/**
 * @brief Allows tracking the completion status and progress of a set of jobs.
 * This can be used to synchronize jobs by making them wait until others have
 * finished. Basically, unfinished jobs increment the counter, while finished
 * jobs decrement it. A counter of value 0 means that all of its jobs have
 * finished, while a counter of value 3 means that 3 jobs are still running.
 */
class JobCounter : public IJobWaitable {
private:
  std::atomic<size_t> m_count{0};

public:
  /**
   * @brief Increment the job counter
   */
  void Increase();

  /**
   * @brief Decrement the job counter. This usually happens, when a job has
   * finished.
   */
  void Decrease();

  /**
   * @brief Checks if any jobs that contribute to the counter are still running,
   * i.e. if the counter is zero.
   * @return true, if all of the counters jobs have finished.
   */
  bool IsFinished() override;
};

inline void JobCounter::Increase() { m_count++; }
inline void JobCounter::Decrease() { m_count--; }
inline bool JobCounter::IsFinished() { return m_count < 1; }

typedef std::shared_ptr<jobsystem::job::JobCounter> SharedJobCounter;

} // namespace jobsystem::job

#endif /* JOBCOUNTER_H */
