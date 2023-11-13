#ifndef TIMERJOB_H
#define TIMERJOB_H

#include "Job.h"
#include "jobsystem/JobSystem.h"
#include <chrono>

namespace jobsystem::job {

/**
 * @brief While regular jobs are executed right away in the next execution
 * cycle, this job has the ability to wait for a certain amount of time. This is
 * useful to create jobs which execute in certain time-intercals instead of
 * every cycle.
 */
class JOBSYSTEM_API TimerJob : public Job {
private:
  /**
   * @brief Point in time where the timer was started. This is not the creation
   * of the job (constructor), but the first attempt of scheduling it.
   */
  std::chrono::time_point<std::chrono::high_resolution_clock> m_timer_start;

  /**
   * @brief Duration that has to pass for the job to get scheduled.
   */
  std::chrono::duration<double> m_time;

  /**
   * @brief If the timer has been started already.
   */
  bool m_timer_started{false};

public:
  TimerJob(std::function<JobContinuation(JobContext *)>, const std::string &id,
           std::chrono::duration<double> time, JobExecutionPhase phase = MAIN);
  TimerJob(std::function<JobContinuation(JobContext *)>,
           std::chrono::duration<double> time, JobExecutionPhase phase = MAIN);
  ~TimerJob() override = default;

  /**
   * @brief Restarts the timer by resetting the start point
   */
  void RestartTimer();

  bool IsReadyForExecution(const JobContext &context) final;
};
} // namespace jobsystem::job

#endif /* TIMERJOB_H */
