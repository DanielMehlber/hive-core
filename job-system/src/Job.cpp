#include "jobsystem/job/Job.h"

using namespace jobsystem::job;

JobContinuation Job::Execute() {
  m_current_state = IN_EXECUTION;
  JobContinuation continuation = m_workload();
  m_current_state = EXECUTION_FINISHED;
  FinishJob();
  return continuation;
}

void Job::AddCounter(std::shared_ptr<JobCounter> counter) {
  counter->Increase();
  m_counters.push_back(counter);
}

Job::Job(std::function<JobContinuation()> workload, std::string name,
         JobExecutionPhase phase)
    : m_workload{workload}, m_name{name}, m_phase{phase} {}

Job::Job(std::function<JobContinuation()> workload, JobExecutionPhase phase)
    : m_workload{workload}, m_phase{phase} {
  static size_t job_count;
  m_name = "job-";
  m_name.append(std::to_string(job_count));
}

void Job::FinishJob() {
  while (!m_counters.empty()) {
    auto counter = m_counters.back();
    m_counters.pop_back();
    counter->Decrease();
  }
}