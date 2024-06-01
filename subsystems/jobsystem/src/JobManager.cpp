#include "jobsystem/manager/JobManager.h"
#include "boost/core/demangle.hpp"
#include "common/profiling/Timer.h"
#include "common/profiling/TimerManager.h"
#include <sstream>

using namespace jobsystem;
using namespace jobsystem::job;
using namespace std::chrono_literals;

JobManager::JobManager(const common::config::SharedConfiguration &config)
    : m_config(config), m_execution(config) {
#ifndef NDEBUG
  auto stats_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        PrintStatusLog();
        return JobContinuation::REQUEUE;
      },
      "print-job-system-stats", 1s);
  KickJob(stats_job);
#endif

#ifdef ENABLE_PROFILING
  auto profiler_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        auto timer_manager = common::profiling::TimerManager::GetGlobal();
        LOG_DEBUG("\n" << timer_manager->Print())
        return JobContinuation::REQUEUE;
      },
      "print-profiler-stats", 3s);
  KickJob(profiler_job);
#endif

  std::stringstream ss;
  ss << "Job System started using "
     << boost::core::demangle(typeid(m_execution).name())
     << " as implementation";

  LOG_INFO(ss.str())
}

JobManager::~JobManager() { m_execution.Stop(); }

void JobManager::PrintStatusLog() {
  LOG_DEBUG(std::to_string(m_cycles_counter) + " cycles completed " +
            std::to_string(m_job_execution_counter) + " jobs")
  // reset debug values
#ifndef NDEBUG
  m_cycles_counter = 0;
  m_job_execution_counter = 0;
#endif
}

void JobManager::KickJob(const SharedJob &job) {

  /*
   * When the job manager is told to detach a job (that is currently in
   * execution), it adds its ID to the requeue blacklist. This prohibits the job
   * execution to requeue the job once it's done because the job-manager
   * intercepts and disposes the re-entering job.
   *
   * In some (rare) scenarios it can occur that a job is detached and kicked in
   * the same execution cycle (e.g. when a job replaces another one). This would
   * not be possible because the manager prohibits scheduling the kicked job
   * because its old counterpart is still on the blacklist. Therefore, a newly
   * kicked job must be removed from the blacklist.
   */
  std::unique_lock lock(m_continuation_requeue_blacklist_mutex);
  bool contains_job = m_continuation_requeue_blacklist.find(job->GetId()) !=
                      m_continuation_requeue_blacklist.end();
  if (contains_job) {
    m_continuation_requeue_blacklist.erase(job->GetId());
  }
  lock.unlock();

  switch (job->GetPhase()) {
  case INIT: {
    {
      std::unique_lock queue_lock(m_init_queue_mutex);
      m_init_queue.push(job);
    }

    std::unique_lock state_lock(m_current_state_mutex);
    if (m_current_state == CYCLE_INIT) {
      ScheduleAllJobsInQueue(m_init_queue, m_init_queue_mutex,
                             m_init_phase_counter);
      DEBUG_ASSERT(m_current_state == CYCLE_INIT,
                   "state is not properly synchronized");
    }
  } break;
  case MAIN: {
    {
      std::unique_lock queue_lock(m_main_queue_mutex);
      m_main_queue.push(job);
    }

    std::unique_lock state_lock(m_current_state_mutex);
    if (m_current_state == CYCLE_MAIN) {
      ScheduleAllJobsInQueue(m_main_queue, m_main_queue_mutex,
                             m_main_phase_counter);
      DEBUG_ASSERT(m_current_state == CYCLE_MAIN,
                   "state is not properly synchronized");
    }
  } break;
  case CLEAN_UP: {
    {
      std::unique_lock queue_lock(m_clean_up_queue_mutex);
      m_clean_up_queue.push(job);
    }

    std::unique_lock state_lock(m_current_state_mutex);
    if (m_current_state == CYCLE_CLEAN_UP) {
      ScheduleAllJobsInQueue(m_clean_up_queue, m_clean_up_queue_mutex,
                             m_clean_up_phase_counter);
      DEBUG_ASSERT(m_current_state == CYCLE_CLEAN_UP,
                   "state is not properly synchronized");
    }
  } break;
  }

  job->SetState(JobState::QUEUED);
}

void JobManager::ScheduleAllJobsInQueue(std::queue<SharedJob> &queue,
                                        recursive_mutex &queue_mutex,
                                        const SharedJobCounter &counter) {
  std::unique_lock lock(queue_mutex);
  while (!queue.empty()) {
    auto job = queue.front();
    queue.pop();

    // if job is not ready yet, queue it for next cycle
    JobContext context(m_total_cycle_count, BorrowFromThis());
    if (!job->IsReadyForExecution(context)) {
      KickJobForNextCycle(job);
      continue;
    }

    // some jobs are long-running and should not be waited for
    bool cycle_should_wait_for_completion = !job->IsAsync();
    if (cycle_should_wait_for_completion) {
      job->AddCounter(counter);
    }

    m_execution.Schedule(job);
#ifndef NDEBUG
    m_job_execution_counter++;
#endif
  }
}

void JobManager::ExecuteQueueAndWait(std::queue<SharedJob> &queue,
                                     recursive_mutex &queue_mutex,
                                     const SharedJobCounter &counter) {

  {
    std::unique_lock lock(queue_mutex);
    if (queue.empty()) {
      return /* because there is nothing to queue */;
    }
  }

  ScheduleAllJobsInQueue(queue, queue_mutex, counter);
  WaitForCompletion(counter);
}

size_t jobsystem::JobManager::GetTotalCyclesCount() const {
  return m_total_cycle_count;
}

void JobManager::ResetContinuationRequeueBlacklist() {
  std::unique_lock lock(m_continuation_requeue_blacklist_mutex);
  m_continuation_requeue_blacklist.clear();
}

void JobManager::InvokeCycleAndWait() {
#ifdef ENABLE_PROFILING
  common::profiling::Timer cycle_timer("job-cycles");
#endif

  // clear black-list for upcoming cycle
  ResetContinuationRequeueBlacklist();

  // put waiting jobs into queues for the upcoming cycle
  std::unique_lock next_cycle_queue_lock(m_next_cycle_queue_mutex);
  while (!m_next_cycle_queue.empty()) {
    auto job = m_next_cycle_queue.front();
    m_next_cycle_queue.pop();
    KickJob(job);
  }
  next_cycle_queue_lock.unlock();

  // start the cycle by starting the execution
  m_total_cycle_count++;

  m_init_phase_counter = std::make_shared<JobCounter>();
  m_main_phase_counter = std::make_shared<JobCounter>();
  m_clean_up_phase_counter = std::make_shared<JobCounter>();

  // pass different phases consecutively to the execution
  {
    std::unique_lock lock(m_current_state_mutex);
    m_current_state = CYCLE_INIT;
  }
  ExecuteQueueAndWait(m_init_queue, m_init_queue_mutex, m_init_phase_counter);

  {
    std::unique_lock lock(m_current_state_mutex);
    m_current_state = CYCLE_MAIN;
  }
  ExecuteQueueAndWait(m_main_queue, m_main_queue_mutex, m_main_phase_counter);

  {
    std::unique_lock lock(m_current_state_mutex);
    m_current_state = CYCLE_CLEAN_UP;
  }
  ExecuteQueueAndWait(m_clean_up_queue, m_clean_up_queue_mutex,
                      m_clean_up_phase_counter);

  {
    std::unique_lock lock(m_current_state_mutex);
    m_current_state = READY;
  }

  ResetContinuationRequeueBlacklist();
#ifndef NDEBUG
  m_cycles_counter++;
#endif
}

void JobManager::KickJobForNextCycle(const SharedJob &job) {
  /*
   * When a job is detached, intercept it from re-entering the job queue.
   */
  std::unique_lock black_list_lock(m_continuation_requeue_blacklist_mutex);
  bool contains_job = m_continuation_requeue_blacklist.find(job->GetId()) !=
                      m_continuation_requeue_blacklist.end();
  if (contains_job) {
    return;
  }
  black_list_lock.unlock();

  std::unique_lock lock(m_next_cycle_queue_mutex);
  m_next_cycle_queue.push(job);
  job->SetState(RESERVED_FOR_NEXT_CYCLE);
}

void tryRemoveJobWithIdFromQueue(const std::string &id,
                                 std::queue<SharedJob> &queue,
                                 recursive_mutex &mutex) {
  std::queue<SharedJob> new_queue;
  std::unique_lock lock(mutex);
  while (!queue.empty()) {
    auto job = queue.front();
    queue.pop();
    bool should_remove_job = job->GetId() == id;
    if (!should_remove_job) {
      new_queue.push(job);
    } else {
      LOG_DEBUG("job " << job->GetId() << " has been detached")
      job->SetState(DETACHED);
    }
  }

  queue.swap(new_queue);
}

void JobManager::DetachJob(const std::string &job_id) {
  /*
   * In order to prohibit jobs from re-entering queues after their execution
   * (see JobContinuation), we remember their id and black-list them.
   */
  std::unique_lock lock(m_continuation_requeue_blacklist_mutex);
  m_continuation_requeue_blacklist.insert(job_id);
  lock.unlock();

  /*
   * try to remove the job from all queues (only works if it is not currently in
   * execution.
   */
  tryRemoveJobWithIdFromQueue(job_id, m_next_cycle_queue,
                              m_next_cycle_queue_mutex);
  tryRemoveJobWithIdFromQueue(job_id, m_init_queue, m_init_queue_mutex);
  tryRemoveJobWithIdFromQueue(job_id, m_main_queue, m_main_queue_mutex);
  tryRemoveJobWithIdFromQueue(job_id, m_clean_up_queue, m_clean_up_queue_mutex);
}

void JobManager::StartExecution() { m_execution.Start(BorrowFromThis()); }
void JobManager::StopExecution() { m_execution.Stop(); }
