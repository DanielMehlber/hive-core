#pragma once

#include "common/assert/Assert.h"
#include "jobsystem/synchronization/IJobWaitable.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <memory>
#include <mutex>

namespace hive::jobsystem::job {

/**
 * Allows tracking the completion status and progress of a set of jobs.
 * This can be used to synchronize jobs by making them wait until others have
 * finished. Basically, unfinished jobs increment the counter, while finished
 * jobs decrement it. A counter of value 0 means that all of its jobs have
 * finished, while a counter of value 3 means that 3 jobs are still running.
 * @note This is the main synchronization primitive used in the job system
 */
class JobCounter : public IJobWaitable {
private:
  /** Current count of unfinished jobs attached to this counter. */
  size_t m_count{0};
  mutable jobsystem::mutex m_count_mutex;

public:
  /**
   * Increment the job counter
   */
  void Increase();

  /**
   * Decrement the job counter. This usually happens, when a job has
   * finished.
   */
  void Decrease();

  /**
   * Checks if any jobs that contribute to the counter are still running,
   * i.e. if the counter is zero.
   * @return true, if all of the counters jobs have finished.
   */
  bool IsFinished() override;
};

inline void JobCounter::Increase() {
  std::unique_lock lock(m_count_mutex);
  m_count++;
}

inline void JobCounter::Decrease() {
  std::unique_lock lock(m_count_mutex);
  DEBUG_ASSERT(m_count > 0, "job counter must not be decreased below zero.")
  m_count--;
}

inline bool JobCounter::IsFinished() {
  std::unique_lock lock(m_count_mutex);
  return m_count < 1;
}

typedef std::shared_ptr<jobsystem::job::JobCounter> SharedJobCounter;

} // namespace hive::jobsystem::job
