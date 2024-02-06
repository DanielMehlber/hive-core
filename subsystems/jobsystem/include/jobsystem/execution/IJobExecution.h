#ifndef IJOBEXECUTION_H
#define IJOBEXECUTION_H

#include "JobExecutionState.h"
#include "jobsystem/job/Job.h"
#include "jobsystem/synchronization/IJobWaitable.h"
#include "jobsystem/synchronization/JobCounter.h"
#include <future>
#include <memory>

using namespace jobsystem::job;

namespace jobsystem::execution {

/**
 * Executes jobs passed by the job manager.
 * @note This interface has been written using the CRTP Pattern to avoid runtime
 * cost of v-tables in the hot path. Virtual function calls are avoided.
 */
template <typename Impl> class IJobExecution {
public:
  /**
   * Schedules the job for execution
   * @param job job to be executed.
   */
  void Schedule(std::shared_ptr<Job> job);

  /**
   * Wait for the waitable object to finish before execution is
   * continued. The implementation can vary depending on the underlying
   * synchronization primitives.
   * @attention Some implementations do not support this call from inside a
   * running job (e.g. the single threaded implementation because this would
   * block the only execution thread)
   * @param waitable process that needs to finish before execution of the
   * calling party can continue.
   */
  void WaitForCompletion(std::shared_ptr<IJobWaitable> waitable);

  /**
   * Execution of the calling party will wait (or will be deferred,
   * depending on the execution environment) until the passed future has been
   * resolved.
   * @tparam FutureType type of the future object
   * @param future future that must resolve in order for the calling party to
   * continue.
   */
  template <typename FutureType>
  void WaitForCompletion(const std::future<FutureType> &future);

  /**
   * Starts processing scheduled jobs and invoke the execution
   * @param manager managing instance that started the execution
   */
  void Start(std::weak_ptr<JobManager> manager);

  /**
   * Stops processing scheduled jobs and pauses the execution
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
template <typename FutureType>
inline void
IJobExecution<Impl>::WaitForCompletion(const std::future<FutureType> &fut) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->WaitForCompletion(fut);
}

template <typename Impl>
inline void IJobExecution<Impl>::Start(std::weak_ptr<JobManager> manager) {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->StartExecution(manager);
}

template <typename Impl> inline void IJobExecution<Impl>::Stop() {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  static_cast<Impl *>(this)->StopExecution();
}

template <typename Impl>
inline JobExecutionState IJobExecution<Impl>::GetState() {
  // CRTP pattern: avoid runtime cost of v-tables in hot path
  // Your implementation of IJobExecution must implement this function
  return static_cast<Impl *>(this)->GetState();
}

} // namespace jobsystem::execution

#endif /* IJOBEXECUTION_H */
