#ifndef IJOBEXECUTION_H
#define IJOBEXECUTION_H

#include "JobExecutionState.h"
#include "jobsystem/job/Job.h"
#include "jobsystem/job/JobCounter.h"
#include <memory>

using namespace jobsystem::job;

namespace jobsystem::execution {

/*
 * This interface has been written using the CRTP Pattern to avoid runtime cost
 * of v-tables in the hot path. Virtual function calls are avoided.
 */
template <typename Impl> class IJobExecution {
public:
  /**
   * @brief Schedules the job for execution.
   * @param job job to be executed.
   */
  void Schedule(std::shared_ptr<Job> job);

  /**
   * @brief Wait for the counter to become 0. The implementation can vary
   * depending on the underlying synchronization primitives.
   * @param counter counter to wait for
   */
  void WaitForCompletion(std::shared_ptr<JobCounter> counter);

  /**
   * @brief Starts processing scheduled jobs
   */
  void Start();

  /**
   * @brief Stops processing scheduled jobs. Already scheduled jobs will remain
   * scheduled.
   */
  void Stop();

  JobExecutionState GetState();
};

template <typename Impl>
inline void IJobExecution<Impl>::Schedule(std::shared_ptr<Job> job) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->Schedule(job);
}

template <typename Impl>
inline void
IJobExecution<Impl>::WaitForCompletion(std::shared_ptr<JobCounter> counter) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->WaitForCompletion(counter);
}

template <typename Impl> inline void IJobExecution<Impl>::Start() {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->Start();
}

template <typename Impl> inline void IJobExecution<Impl>::Stop() {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->Stop();
}

template <typename Impl>
inline JobExecutionState IJobExecution<Impl>::GetState() {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  return static_cast<Impl *>(this)->GetState();
}

} // namespace jobsystem::execution

#endif /* IJOBEXECUTION_H */
