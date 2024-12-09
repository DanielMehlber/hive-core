#pragma once

#include "JobExecutionState.h"
#include "common/config/Configuration.h"
#include "jobsystem/jobs/Job.h"
#include <future>
#include <memory>

namespace hive::jobsystem::execution {

/**
 * Executes jobs passed by the job manager.
 * @note This is an interface which might have multiple compile-time
 * implementations.
 */
class JobExecution {

  /**
   * Pointer to implementation (Pimpl) in source file. This is necessary to
   * constrain the underlying library's implementation details in the source
   * file and not expose them to the rest of the application.
   * @note Indispensable for ABI stability and to use static-linked libraries.
   */
  struct Impl;
  std::unique_ptr<Impl> m_impl;

  /** Current state of the job execution */
  JobExecutionState m_current_state = STOPPED;

public:
  explicit JobExecution(const common::config::SharedConfiguration &config);
  ~JobExecution();

  JobExecution(const JobExecution &other) = delete;
  JobExecution &operator=(const JobExecution &other) = delete;

  JobExecution(JobExecution &&other) noexcept;
  JobExecution &operator=(JobExecution &&other) noexcept;

  /**
   * Schedules the job for execution. The job will be handed over to the
   * execution and executed shortly.
   * @param job job to be executed.
   */
  void Schedule(const std::shared_ptr<Job> &job);

  /**
   * Allows a job to yield its execution, letting other jobs run instead. The
   * job is paused and resumed later, enabling cooperative multitasking.
   *
   * Useful for asynchronous operations where a job waits for results, avoiding
   * resource blocking by passing control to others, allowing them to make
   * progress instead.
   *
   * @note Enables asynchronous behavior in job execution.
   * @attention Must support both jobs and regular threads. Regular threads
   * should also yield on the thread-level.
   */
  void YieldToWaitingJobs();

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
};

} // namespace hive::jobsystem::execution
