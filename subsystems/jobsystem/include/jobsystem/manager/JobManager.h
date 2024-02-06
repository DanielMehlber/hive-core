#ifndef JOBMANAGER_H
#define JOBMANAGER_H

// *** required for windows ***
#undef max
// MSVC defines a macro called 'max' in windows.h,
// clashing with method names in the boost library.

#include "JobManagerState.h"
#include "common/config/Configuration.h"
#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/execution/IJobExecution.h"
#include "jobsystem/job/Job.h"
#include "jobsystem/job/TimerJob.h"
#include "jobsystem/synchronization/JobMutex.h"
#include "logging/LogManager.h"
#include <set>
#include <utility>

#ifdef JOB_SYSTEM_SINGLE_THREAD
#include "jobsystem/execution/impl/singleThreaded/SingleThreadedExecutionImpl.h"
typedef impl::SingleThreadedExecutionImpl JobExecutionImpl;
#else
#include "jobsystem/execution/impl/fiber/FiberExecutionImpl.h"
typedef jobsystem::execution::impl::FiberExecutionImpl JobExecutionImpl;
#endif

namespace jobsystem {

/**
 * Manages the job processing order, controls the progress of cycles and
 * holds all job instances that must be executed in the current or following
 * execution cycles.
 */
class JobManager : public std::enable_shared_from_this<JobManager> {
private:
  common::config::SharedConfiguration m_config;

  /**
   * All jobs for the initialization phase of the cycle are collected
   * here.
   */
  std::queue<SharedJob> m_init_queue;
  mutable mutex m_init_queue_mutex;
  SharedJobCounter m_init_phase_counter;

  /**
   * All jobs for the main processing phase of the cycle are collected
   * here.
   */
  std::queue<SharedJob> m_main_queue;
  mutable mutex m_main_queue_mutex;
  SharedJobCounter m_main_phase_counter;

  /**
   * All jobs for the clean-up phase of the cycle are collected here.
   */
  std::queue<SharedJob> m_clean_up_queue;
  mutable mutex m_clean_up_queue_mutex;
  SharedJobCounter m_clean_up_phase_counter;

  /**
   * All jobs that should not be kicked for the following cycles instead
   * for the currently running cycles will be collected here. This is the case,
   * when a job is automatically rescheduled for future cycles (e.g.
   * time-interval jobs).
   */
  std::queue<SharedJob> m_next_cycle_queue;
  mutable mutex m_next_cycle_queue_mutex;

  /**
   * Some jobs must be prevented from being rescheduled for the next
   * cycle. This is often the case when trying to detach jobs which are
   * currently in the execution and must therefore be prevented from requeueing.
   */
  std::set<std::string> m_continuation_requeue_blacklist;
  mutable mutex m_continuation_requeue_blacklist_mutex;

  JobManagerState m_current_state{READY};
  JobExecutionImpl m_execution;

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
  void ExecuteQueueAndWait(std::queue<SharedJob> &queue, mutex &queue_mutex,
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
  void ScheduleAllJobsInQueue(std::queue<SharedJob> &queue, mutex &queue_mutex,
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
   * Starts the job system execution cycle.
   * @note This must be called after the job system has been initialized.
   */
  void StartExecution();

  /**
   * Stops the job system execution cycle.
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
   * executed (again)
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
   * Starts a new execution cycle and passes queued jobs to the
   * execution. The calling thread will be blocked until the cycle has finished.
   */
  void InvokeCycleAndWait();

  /**
   * Execution will wait (or will be deferred, depending on the execution
   * environment) until the waitable object has been finished.
   * @param waitable process that needs finish for the current calling party to
   * continue.
   * @note On single-threaded implementations, this cannot be called from inside
   * a job because it would deadlock the worker thread.
   */
  void WaitForCompletion(std::shared_ptr<IJobWaitable> counter);

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

  size_t GetTotalCyclesCount() const noexcept;
};

inline void
JobManager::WaitForCompletion(std::shared_ptr<IJobWaitable> counter) {
  m_execution.WaitForCompletion(std::move(counter));
}

template <typename FutureType>
inline void
JobManager::WaitForCompletion(const std::future<FutureType> &future) {
  m_execution.WaitForCompletion(future);
}

typedef std::shared_ptr<JobManager> SharedJobManager;

} // namespace jobsystem

#endif /* JOBMANAGER_H */
