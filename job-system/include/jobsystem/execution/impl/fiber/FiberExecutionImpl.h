#ifndef FIBEREXECUTIONIMPL_H
#define FIBEREXECUTIONIMPL_H

#include "boost/fiber/all.hpp"
#include "jobsystem/execution/IJobExecution.h"
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

using namespace jobsystem::execution;

namespace jobsystem::execution::impl {
class FiberExecutionImpl : IJobExecution<FiberExecutionImpl> {
private:
  JobExecutionState m_current_state{STOPPED};

  // const size_t m_worker_thread_count{std::thread::hardware_concurrency() -
  // 1};
  const size_t m_worker_thread_count{2};

  /**
   * @brief Contains all worker threads that run scheduled fibers.
   */
  std::vector<std::shared_ptr<std::thread>> m_worker_threads;

  std::unique_ptr<boost::fibers::buffered_channel<std::function<void()>>>
      m_job_channel;

  /**
   * @brief Managing instance necessary to execute jobs and build the
   * JobContext.
   * @note This is set when starting the execution and cleared when the
   * execution has stopped.
   */
  JobManager *m_managing_instance;

  void Init();
  void ShutDown();

  /**
   * @brief Processes jobs as fibers.
   * @note This is run by the worker threads
   */
  void
  ExecuteWorker(std::shared_ptr<boost::fibers::barrier> m_init_sync_barrier);

public:
  FiberExecutionImpl();
  virtual ~FiberExecutionImpl();

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
   * @param counter counter to wait for
   */
  void WaitForCompletion(std::shared_ptr<JobCounter> counter);

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

inline JobExecutionState FiberExecutionImpl::GetState() {
  return m_current_state;
}

} // namespace jobsystem::execution::impl

#endif /* FIBEREXECUTIONIMPL_H */
