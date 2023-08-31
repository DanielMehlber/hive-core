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
  case INIT: {
    std::unique_lock queue_lock(m_init_queue_mutex);
    m_init_queue.push(job);
    queue_lock.unlock();

    // if current cycle is running in this phase, directly schedule it.
    if (m_current_state == CYCLE_INIT) {
      ScheduleAllJobsInQueue(m_init_queue, m_init_queue_mutex,
                             m_init_phase_counter);
    }
  } break;
  case MAIN: {
    std::unique_lock queue_lock(m_main_queue_mutex);
    m_main_queue.push(job);
    queue_lock.unlock();

    // if current cycle is running in this phase, directly schedule it.
    if (m_current_state == CYCLE_MAIN) {
      ScheduleAllJobsInQueue(m_main_queue, m_main_queue_mutex,
                             m_main_phase_counter);
    }
  } break;
  case CLEAN_UP: {
    std::unique_lock queue_lock(m_clean_up_queue_mutex);
    m_clean_up_queue.push(job);
    queue_lock.unlock();

    // if current cycle is running in this phase, directly schedule it.
    if (m_current_state == CYCLE_CLEAN_UP) {
      ScheduleAllJobsInQueue(m_clean_up_queue, m_clean_up_queue_mutex,
                             m_clean_up_phase_counter);
    }
  } break;
  }

  job->SetState(JobState::QUEUED);
}

void JobManager::ScheduleAllJobsInQueue(std::queue<std::shared_ptr<Job>> &queue,
                                        std::mutex &queue_mutex,
                                        std::shared_ptr<JobCounter> counter) {
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

    job->AddCounter(counter);
    m_execution.Schedule(job);
#ifndef NDEBUG
    m_job_execution_counter++;
#endif
  }
}

void JobManager::ExecuteQueueAndWait(std::queue<std::shared_ptr<Job>> &queue,
                                     std::mutex &queue_mutex,
                                     std::shared_ptr<JobCounter> counter) {
  ScheduleAllJobsInQueue(queue, queue_mutex, counter);
  WaitForCompletion(counter);
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

  m_init_phase_counter = std::make_shared<JobCounter>();
  m_main_phase_counter = std::make_shared<JobCounter>();
  m_clean_up_phase_counter = std::make_shared<JobCounter>();

  // pass different phases consecutively to the execution
  m_current_state = CYCLE_INIT;
  ExecuteQueueAndWait(m_init_queue, m_init_queue_mutex, m_init_phase_counter);

  m_current_state = CYCLE_MAIN;
  ExecuteQueueAndWait(m_main_queue, m_main_queue_mutex, m_main_phase_counter);

  m_current_state = CYCLE_CLEAN_UP;
  ExecuteQueueAndWait(m_clean_up_queue, m_clean_up_queue_mutex,
                      m_clean_up_phase_counter);

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