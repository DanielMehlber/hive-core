#pragma once

#include "common/config/Configuration.h"
#include "jobsystem/execution/IJobExecution.h"
#include <condition_variable>
#include <future>
#include <memory>
#include <thread>
#include <vector>

// include order matters here
#include "boost/fiber/all.hpp"

namespace jobsystem::execution::impl {

/**
 * This implementation of the job execution uses a concept called fibers,
 * which (in constrast to threads) are scheduled by a scheduler cooperatively,
 * not preemptively. In this implementation, fibers are provided by the
 * Boost.Fiber library.
 * @note Fibers are running on a pool of worker threads which exchange them to
 * share their work. When a job yields, the fiber it's running on might be
 * executed on a different thread after it has been revoked as last time.
 */
class BoostFiberExecution
    : jobsystem::execution::IJobExecution<BoostFiberExecution> {
private:
  common::config::SharedConfiguration m_config;
  jobsystem::execution::JobExecutionState m_current_state{STOPPED};

  int m_worker_thread_count;

  /**
   * Contains all worker threads that run scheduled fibers.
   */
  std::vector<std::shared_ptr<std::thread>> m_worker_threads;

  std::unique_ptr<boost::fibers::buffered_channel<std::function<void()>>>
      m_job_channel;

  /**
   * Managing instance necessary to execute jobs and build the
   * JobContext.
   * @note This is set when starting the execution and cleared when the
   * execution has stopped.
   */
  common::memory::Reference<JobManager> m_managing_instance;

  void Init();
  void ShutDown();

  /**
   * Processes jobs as fibers.
   * @param barrier Is notified when the main fiber has been set up and is ready
   * to run.
   * @note This is run by the worker threads.
   */
  void ExecuteWorker(std::atomic_int *barrier);

public:
  BoostFiberExecution() = delete;
  explicit BoostFiberExecution(
      const common::config::SharedConfiguration &config);
  virtual ~BoostFiberExecution();

  /**
   * Schedules the job for execution
   * @param job job to be executed.
   */
  void Schedule(std::shared_ptr<Job> job);

  /**
   * Wait for the counter to become 0. The implementation can vary
   * depending on the underlying synchronization primitives.
   * @attention Some implementations do not support this call from inside a
   * running job (e.g. the single threaded implementation because this would
   * block the only execution thread)
   * @param waitable counter to wait for
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
  void Start(common::memory::Borrower<JobManager> manager);

  /**
   * Stops processing scheduled jobs and pauses the execution
   * @note Already scheduled jobs will remain scheduled until the execution is
   * resumed.
   */
  void Stop();

  JobExecutionState GetState();
};

/**
 * Checks if current executor is a fiber or a normal thread.
 */
bool IsExecutedByFiber();

template <typename FutureType>
inline void
BoostFiberExecution::WaitForCompletion(const std::future<FutureType> &future) {
  if (IsExecutedByFiber()) {
    // caller is a fiber, so yield
    while (future.wait_for(std::chrono::seconds(0)) !=
           std::future_status::ready) {
      boost::this_fiber::yield();
    }
  } else {
    // caller is a thread, so block
    future.wait();
  }
}

inline jobsystem::execution::JobExecutionState BoostFiberExecution::GetState() {
  return m_current_state;
}

} // namespace jobsystem::execution::impl
