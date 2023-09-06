#ifndef TIMERJOB_H
#define TIMERJOB_H

#include "Job.h"
#include <chrono>

namespace jobsystem::job {
class TimerJob : public Job {
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
  TimerJob(std::function<JobContinuation(JobContext *)>, std::string name,
           std::chrono::duration<double> time, JobExecutionPhase phase = MAIN);
  TimerJob(std::function<JobContinuation(JobContext *)>,
           std::chrono::duration<double> time, JobExecutionPhase phase = MAIN);
  virtual ~TimerJob(){};

  /**
   * @brief Restarts the timer by resetting the start point
   */
  void RestartTimer();

  virtual bool IsReadyForExecution(const JobContext &context) final override;
};
} // namespace jobsystem::job

#endif /* TIMERJOB_H */
