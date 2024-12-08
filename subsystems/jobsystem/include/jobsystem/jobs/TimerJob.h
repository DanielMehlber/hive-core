#pragma once

#include "Job.h"
#include <chrono>

namespace hive::jobsystem {

/**
 * While regular jobs are executed right away in the next execution
 * cycle, this job has the ability to defer execution for a certain amount of
 * time by skipping cycles.
 * @note This is useful to create jobs which execute in certain time-intervals
 * instead of every cycle.
 */
class TimerJob : public Job {
  /**
   * Point in time where the timer was started. This is not the creation
   * of the job (constructor), but the first attempt of scheduling it.
   */
  std::chrono::time_point<std::chrono::high_resolution_clock> m_timer_start;

  /** Duration that has to pass for the job to get scheduled. */
  std::chrono::duration<double> m_time;

  /** If the timer has been started already. */
  bool m_timer_started{false};

public:
  /**
   * Creates a new timer job with a given workload, id, time, phase and
   * synchronization behavior.
   * @param workload function that will be executed by the job.
   * @param id unique identifier of this job.
   * @param time duration that has to pass for the job to get scheduled.
   * @param phase in which the job should be executed during the execution
   * cycle.
   * @param async if the job is asynchronous, the cycle will not wait for it to
   * finish.
   */
  TimerJob(std::function<JobContinuation(JobContext *)> workload,
           const std::string &id, std::chrono::duration<double> time,
           JobExecutionPhase phase = MAIN, bool async = false);

  ~TimerJob() override = default;

  /**
   * Restarts the timer by resetting the start point.
   */
  void RestartTimer();

  bool IsReadyForExecution(const JobContext &context) final;
};
} // namespace hive::jobsystem
