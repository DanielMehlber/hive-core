#include "jobsystem/JobManager.h"

using namespace jobsystem;
using namespace jobsystem::job;

void JobManager::KickJob(std::shared_ptr<Job> job) {
  switch (job->GetPhase()) {
  case INIT:
    m_init_queue.push(job);
    break;
  case MAIN:
    m_main_queue.push(job);
    break;
  case CLEAN_UP:
    m_main_queue.push(job);
    break;
  }

  job->SetState(JobState::QUEUED);
}

void JobManager::ExecutePhaseAndWait(std::queue<std::shared_ptr<Job>> &queue,
                                     std::mutex &queue_mutex) {
  std::unique_lock lock(queue_mutex);
  if (!queue.empty()) {
    auto init_counter = std::make_shared<JobCounter>();
    while (!queue.empty()) {
      auto job = queue.back();
      queue.pop();
      job->AddCounter(init_counter);
      m_execution.Schedule(job);
    }
    lock.unlock();

    WaitForCompletion(init_counter);
  }
}

void JobManager::InvokeCycleAndWait() {
  m_execution.Start();

  ExecutePhaseAndWait(m_init_queue, m_init_queue_mutex);
  ExecutePhaseAndWait(m_main_queue, m_main_queue_mutex);
  ExecutePhaseAndWait(m_clean_up_queue, m_clean_up_queue_mutex);

  m_execution.Stop();
}