#include "jobsystem/execution/impl/singleThreaded/SingleThreadedExecutionImpl.h"
#include "jobsystem/manager/JobManager.h"
#include "logging/Logging.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;

SingleThreadedExecutionImpl::SingleThreadedExecutionImpl() {
  m_current_state = JobExecutionState::STOPPED;
}

SingleThreadedExecutionImpl::~SingleThreadedExecutionImpl() { Stop(); }

void SingleThreadedExecutionImpl::Schedule(const std::shared_ptr<Job> &job) {
  std::unique_lock lock(m_execution_queue_mutex);
  m_execution_queue.push(job);
  job->SetState(JobState::AWAITING_EXECUTION);
  lock.unlock();
  m_execution_queue_condition.notify_one();
}

void SingleThreadedExecutionImpl::ExecuteJobs(JobManager *manager) {
  while (!m_termination_flag) {
    std::unique_lock queue_lock(m_execution_queue_mutex);
    if (!m_execution_queue.empty()) {
      auto job = m_execution_queue.front();
      m_execution_queue.pop();
      queue_lock.unlock();

      // generate job context
      JobContext context(manager->GetTotalCyclesCount(), manager);

      // run job
      JobContinuation continuation = job->Execute(&context);
      job->SetState(JobState::EXECUTION_FINISHED);

      if (continuation == REQUEUE) {
        manager->KickJobForNextCycle(job);
      }

    } else {
      // unlocks queue_lock until notified (see documentation)
      m_execution_queue_condition.wait(queue_lock, [&] {
        return !m_execution_queue.empty() || m_termination_flag;
      });
    }
  }
}

void SingleThreadedExecutionImpl::Start(JobManager *manager) {
  if (m_current_state == JobExecutionState::STOPPED) {
    m_termination_flag = false;
    m_worker_thread = std::make_unique<std::thread>(
        &SingleThreadedExecutionImpl::ExecuteJobs, this, manager);
    m_current_state = JobExecutionState::RUNNING;
  }
}

void SingleThreadedExecutionImpl::Stop() {
  if (m_current_state == JobExecutionState::RUNNING) {
    std::unique_lock lock(m_execution_queue_mutex);
    m_termination_flag = true;
    lock.unlock();
    m_execution_queue_condition.notify_all();
    m_worker_thread->join();
    m_current_state = JobExecutionState::STOPPED;
  }
}

using namespace std::chrono_literals;
void SingleThreadedExecutionImpl::WaitForCompletion(
    std::shared_ptr<IJobWaitable> waitable) {

  if (m_worker_thread->get_id() == std::this_thread::get_id()) {
    LOG_ERR("cannot wait for other jobs during job execution using the "
            "SingleThreadedExecutionImpl because it would block the only "
            "execution thread");
    return;
  }

  while (!waitable->IsFinished()) {
    std::this_thread::sleep_for(1ms);
  }
}
