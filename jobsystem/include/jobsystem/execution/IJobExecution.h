#ifndef IJOBEXECUTION_H
#define IJOBEXECUTION_H

#include "JobExecutionState.h"
#include "jobsystem/job/IJobWaitable.h"
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
   * @brief Schedules the job for execution
   * @param job job to be executed.
   */
  void Schedule(std::shared_ptr<Job> job);

  /**
   * @brief Wait for the counter to become 0. The implementation can vary
   * depending on the underlying synchronization primitives.
   * @attention Some implementations do not support this call from inside a
   * running job (e.g. the single threaded implementation because this would
   * block the only execution thread)
   * @param waitable counter to wait for
   */
  void WaitForCompletion(std::shared_ptr<IJobWaitable> waitable);

  /**
   * @brief Starts processing scheduled jobs and invoke the execution
   * @param manager managing instance that started the execution
   */
  void Start(JobManager *manager);

  /**
   * @brief Stops processing scheduled jobs and pauses the execution
   * @note Already scheduled jobs will remain scheduled until the execution is
   * resumed.
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
IJobExecution<Impl>::WaitForCompletion(std::shared_ptr<IJobWaitable> waitable) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->WaitForCompletion(waitable);
}

template <typename Impl>
inline void IJobExecution<Impl>::Start(JobManager *manager) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->Start(manager);
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
