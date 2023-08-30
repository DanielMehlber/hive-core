#include "jobsystem/JobManager.h"

using namespace jobsystem;
using namespace jobsystem::job;
using namespace std::chrono_literals;

JobManager::JobManager() {
#ifndef NDEBUG
  auto job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        PrintStatusLog();
        return JobContinuation::REQUEUE;
      },
      1s);
  KickJob(job);
#endif
}

void JobManager::PrintStatusLog() {
  LOG_DEBUG(std::to_string(m_cycles_counter) + " cycles completed " +
            std::to_string(m_job_execution_counter) + " jobs");
  // reset debug values
#ifndef NDEBUG
  m_cycles_counter = 0;
  m_job_execution_counter = 0;
#endif
}

void JobManager::KickJob(std::shared_ptr<Job> job) {
  switch (job->GetPhase()) {
  case INIT:
    m_init_queue.push(job);
    break;
  case MAIN:
    m_main_queue.push(job);
    break;
  case CLEAN_UP:
    m_clean_up_queue.push(job);
    break;
  }

  job->SetState(JobState::QUEUED);
}

void JobManager::ExecutePhaseAndWait(std::queue<std::shared_ptr<Job>> &queue,
                                     std::mutex &queue_mutex) {
  if (!queue.empty()) {
    auto init_counter = std::make_shared<JobCounter>();
    std::unique_lock lock(queue_mutex);
    while (!queue.empty()) {
      auto job = queue.front();
      queue.pop();

      // if job is not ready yet, queue it for next cycle
      JobContext context(m_total_cycle_count, this);
      if (!job->IsReadyForExecution(context)) {
        KickJobForNextCycle(job);
        continue;
      }

      job->AddCounter(init_counter);
      m_execution.Schedule(job);
#ifndef NDEBUG
      m_job_execution_counter++;
#endif
    }

    lock.unlock();
    WaitForCompletion(init_counter);
  }
}

size_t jobsystem::JobManager::GetTotalCyclesCount() noexcept {
  return m_total_cycle_count;
}

void JobManager::InvokeCycleAndWait() {
  // put waiting jobs into queues for the upcoming cycle
  std::unique_lock lock(m_next_cycle_queue_mutex);
  while (!m_next_cycle_queue.empty()) {
    auto job = m_next_cycle_queue.front();
    m_next_cycle_queue.pop();
    KickJob(job);
  }
  lock.unlock();

  // start the cycle by starting the execution
  m_total_cycle_count++;
  m_execution.Start(this);

  // pass different phases consecutively to the execution
  ExecutePhaseAndWait(m_init_queue, m_init_queue_mutex);
  ExecutePhaseAndWait(m_main_queue, m_main_queue_mutex);
  ExecutePhaseAndWait(m_clean_up_queue, m_clean_up_queue_mutex);

  m_execution.Stop();

#ifndef NDEBUG
  m_cycles_counter++;
#endif
}

void JobManager::KickJobForNextCycle(std::shared_ptr<Job> job) {
  std::unique_lock lock(m_next_cycle_queue_mutex);
  m_next_cycle_queue.push(job);
  lock.unlock();
}