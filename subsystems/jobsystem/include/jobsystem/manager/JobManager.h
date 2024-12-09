#pragma once

// *** required for windows ***
#undef max
// MSVC defines a macro called 'max' in windows.h,
// clashing with method names in the boost library.

#include "JobManagerState.h"
#include "common/config/Configuration.h"
#include "jobsystem/execution/JobExecution.h"
#include "jobsystem/jobs/Job.h"
#include "jobsystem/synchronization/IJobBarrier.h"
#include "jobsystem/synchronization/JobRecursiveMutex.h"
#include <set>
#include <utility>

namespace hive::jobsystem {

/**
 * Manages the job processing order, controls the progress of cycles and
 * holds all job instances that must be executed in the current or following
 * execution cycles.
 */
class JobManager : public common::memory::EnableBorrowFromThis<JobManager> {
  common::config::SharedConfiguration m_config;

  /**
   * All jobs for the initialization phase of the cycle are collected
   * here.
   */
  std::queue<SharedJob> m_init_queue;
  mutable recursive_mutex m_init_queue_mutex;
  SharedJobCounter m_init_phase_counter;

  /**
   * All jobs for the main processing phase of the cycle are collected
   * here.
   */
  std::queue<SharedJob> m_main_queue;
  mutable recursive_mutex m_main_queue_mutex;
  SharedJobCounter m_main_phase_counter;

  /**
   * All jobs for the clean-up phase of the cycle are collected here.
   */
  std::queue<SharedJob> m_clean_up_queue;
  mutable recursive_mutex m_clean_up_queue_mutex;
  SharedJobCounter m_clean_up_phase_counter;

  /**
   * All jobs that should not be kicked for the following cycles instead
   * for the currently running cycles will be collected here. This is the case,
   * when a job is automatically rescheduled for future cycles (e.g.
   * time-interval jobs).
   */
  std::queue<SharedJob> m_next_cycle_queue;
  mutable recursive_mutex m_next_cycle_queue_mutex;

  /**
   * Some jobs must be prevented from being rescheduled for the next
   * cycle. This is often the case when trying to detach jobs which are
   * currently in the execution and must therefore be prevented from requeueing.
   */
  std::set<std::string> m_continuation_requeue_blacklist;
  mutable mutex m_continuation_requeue_blacklist_mutex;

  JobManagerState m_current_state{READY};
  mutable mutex m_current_state_mutex;

  execution::JobExecution m_execution;

  size_t m_total_cycle_count{0};

// Debug values
#ifndef NDEBUG
  std::atomic<size_t> m_job_execution_counter{0};
  size_t m_cycles_counter{0};
#endif

  /**
   * Pushes all job instances contained in the passed queue to the
   * execution and waits until all have been executed.
   * @param queue queue containing all jobs that should be executed
   * @param queue_mutex mutex of the queue enabling concurrent access and
   * modification
   * @param counter counter that should be used to track the progress of all job
   * instances pushed into the execution
   */
  void ExecuteQueueAndWait(std::queue<SharedJob> &queue,
                           recursive_mutex &queue_mutex,
                           const SharedJobCounter &counter);

  /**
   * Pushes all job instances contained in the passed queue to the
   * execution for scheduling.
   * @param queue queue containing all job instances that should be executed
   * @param queue_mutex mutex of the passed queue enabling concurrent access and
   * modification
   * @param counter counter that is attached to all job instances that get
   * passed to the job execution.
   */
  void ScheduleAllJobsInQueue(std::queue<SharedJob> &queue,
                              recursive_mutex &queue_mutex,
                              const SharedJobCounter &counter);

  /**
   * The continuation requeue blacklist is used when cancelling jobs by
   * preventing their re-queueing. This operation clears the blacklist.
   */
  void ResetContinuationRequeueBlacklist();

public:
  JobManager() = delete;
  explicit JobManager(const common::config::SharedConfiguration &config);
  ~JobManager();
  JobManager(JobManager &other) = delete;

  /**
   * Activate the job system execution. Jobs will be processed when passed to
   * the execution.
   */
  void StartExecution();

  /**
   * Stops the job system execution cycle. Jobs will not be processed, even when
   * passed to the execution. They will simply pile up.
   */
  void StopExecution();

  /**
   * Logs runtime information and statistics.
   */
  void PrintStatusLog();

  /**
   * Pass detached job instance to manager in order to be executed in the
   * current cycle or the next one, if none is currently running.
   * @param job job that should be executed
   * @note The job will be executed when the execution cycle has been invoked
   */
  void KickJob(const SharedJob &job);

  /**
   * Ensures that a job which is not yet in execution will not be
   * executed (again). It will be renoved from the queue or intercepted before
   * it can requeue.
   * @param job_id id of this job
   */
  void DetachJob(const std::string &job_id);

  /**
   * Pass detached job instance to manager in order to be exected in the
   * next cycle (not the current one).
   * @param job job that should be executed
   * @note This is useful for re-queueing jobs for later execution
   */
  void KickJobForNextCycle(const SharedJob &job);

  /**
   * Starts a new execution cycle and passes queued jobs to the execution. The
   * calling thread will be blocked until all synchronous jobs are done.
   * @attention Asynchronous jobs will not be waited for.
   */
  void InvokeCycleAndWait();

  /**
   * Execution will wait until the barrier object has been resolved. The calling
   * job will yield its execution to others in the meantime.
   * @param barrier process that needs finish for the current calling party to
   * continue.
   * @note On single-threaded implementations, this cannot be called from inside
   * a job because it would deadlock the worker thread.
   */
  void Await(std::shared_ptr<IJobBarrier> barrier);

  /**
   * Execution will wait until the future has been resolved. The calling
   * job will yield its execution to others in the meantime.
   * @tparam FutureType type of the future object
   * @param future future that must resolve in order for the calling party to
   * continue.
   */
  template <typename FutureType>
  void Await(const std::future<FutureType> &future);

  /**
   * Execution will wait until the shared future has been resolved. The calling
   * job will yield its execution to others in the meantime.
   * @tparam FutureType type of the future object
   * @param future future that must resolve in order for the calling party to
   * continue.
   */
  template <typename FutureType>
  void Await(const std::shared_future<FutureType> &future);

  /**
   * Wait for a fixed amount of time before continuing the job's execution.
   * @param duration duration to wait before continuing.
   */
  template <typename Rep, typename Period>
  void WaitForDuration(std::chrono::duration<Rep, Period> duration);

  /**
   * Return total count of cycles that have been completed
   * @return total count of cycles
   */
  size_t GetTotalCyclesCount() const;
};

inline void JobManager::Await(std::shared_ptr<IJobBarrier> barrier) {
  while (!barrier->IsFinished()) {
    m_execution.YieldToWaitingJobs();
  }
}

template <typename FutureType>
void JobManager::Await(const std::future<FutureType> &future) {
  while (future.wait_for(std::chrono::milliseconds(0)) !=
         std::future_status::ready) {
    m_execution.YieldToWaitingJobs();
  }
}

template <typename FutureType>
void JobManager::Await(const std::shared_future<FutureType> &future) {
  while (future.wait_for(std::chrono::milliseconds(0)) !=
         std::future_status::ready) {
    m_execution.YieldToWaitingJobs();
  }
}

template <typename Rep, typename Period>
void JobManager::WaitForDuration(std::chrono::duration<Rep, Period> duration) {
  const auto start = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - start < duration) {
    m_execution.YieldToWaitingJobs();
  }
}

} // namespace hive::jobsystem
