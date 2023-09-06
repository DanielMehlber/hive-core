#include "jobsystem/job/TimerJob.h"

using namespace jobsystem::job;

TimerJob::TimerJob(std::function<JobContinuation(JobContext *)> workload,
                   const std::string &id, std::chrono::duration<double> time,
                   JobExecutionPhase phase)
    : Job(workload, id), m_time{time} {}

TimerJob::TimerJob(std::function<JobContinuation(JobContext *)> workload,
                   std::chrono::duration<double> time, JobExecutionPhase phase)
    : Job(workload), m_time{time} {}

void TimerJob::RestartTimer() {
  m_timer_start = std::chrono::high_resolution_clock::now();
  m_timer_started = true;
}

bool TimerJob::IsReadyForExecution(const JobContext &context) {
  // Start timer at first attempt of scheduling, not job creation
  if (!m_timer_started) {
    RestartTimer();
  }

  auto now = std::chrono::high_resolution_clock::now();
  auto duration = now - m_timer_start;
  bool time_passed = duration >= m_time;

  if (time_passed) {
    // causes timer to reset for next iteration (in case this job is
    // rescheduled)
    m_timer_started = false;
  }
  return time_passed;
}