#include "jobsystem/job/Job.h"
#include "common/uuid/UuidGenerator.h"
#include "logging/LogManager.h"

#ifdef ENABLE_PROFILING
#include "common/profiling/Timer.h"
#endif

using namespace jobsystem::job;

JobContinuation Job::Execute(JobContext *context) {
#ifdef ENABLE_PROFILING
  common::profiling::Timer job_execution_timer("job-execution-" + m_id);
#endif
  m_current_state = IN_EXECUTION;
  JobContinuation continuation;
  try {
    continuation = m_workload(context);
    m_current_state = EXECUTION_FINISHED;
  } catch (const std::exception &exception) {
    LOG_ERR("job " << m_id
                   << " threw and exception and failed: " << exception.what())
    m_current_state = FAILED;
    continuation = DISPOSE;
  }

  return continuation;
}

void Job::AddCounter(const std::shared_ptr<JobCounter> &counter) {
  std::unique_lock counter_lock(m_counters_mutex);
  counter->Increase();
  m_counters.push(counter);
}

Job::Job(std::function<JobContinuation(JobContext *)> workload, std::string id,
         JobExecutionPhase phase, bool async)
    : m_workload{std::move(workload)}, m_id{std::move(id)}, m_phase{phase},
      m_async{async} {}

void Job::FinishJob() {
  std::unique_lock counter_lock(m_counters_mutex);
  while (!m_counters.empty()) {
    auto counter = m_counters.front();
    m_counters.pop();
    counter->Decrease();
  }
}