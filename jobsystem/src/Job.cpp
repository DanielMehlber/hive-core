#include "jobsystem/job/Job.h"
#include "logging/Logging.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace jobsystem::job;

JobContinuation Job::Execute(JobContext *context) {
  m_current_state = IN_EXECUTION;
  JobContinuation continuation = m_workload(context);
  m_current_state = EXECUTION_FINISHED;
  FinishJob();

  return continuation;
}

void Job::AddCounter(std::shared_ptr<JobCounter> counter) {
  counter->Increase();
  m_counters.push_back(counter);
}

Job::Job(std::function<JobContinuation(JobContext *)> workload,
         const std::string &id, JobExecutionPhase phase)
    : m_workload{workload}, m_id{id}, m_phase{phase} {}

Job::Job(std::function<JobContinuation(JobContext *)> workload,
         JobExecutionPhase phase)
    : m_workload{workload}, m_phase{phase},
      m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {}

void Job::FinishJob() {
  while (!m_counters.empty()) {
    auto counter = m_counters.back();
    m_counters.pop_back();
    counter->Decrease();
  }
}