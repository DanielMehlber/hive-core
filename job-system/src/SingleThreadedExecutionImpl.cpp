#include "jobsystem/execution/impl/singleThreaded/SingleThreadedExecutionImpl.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;

SingleThreadedExecutionImpl::SingleThreadedExecutionImpl() {
  m_current_state = JobExecutionState::STOPPED;
}

SingleThreadedExecutionImpl::~SingleThreadedExecutionImpl() { Stop(); }

void SingleThreadedExecutionImpl::Schedule(std::shared_ptr<Job> job) {
  std::unique_lock lock(m_queue_mutex);
  m_queue.push(job);
  job->SetState(JobState::AWAITING_EXECUTION);
  lock.unlock();
  m_queue_condition.notify_one();
}

void SingleThreadedExecutionImpl::ExecuteJobs() {
  while (!m_should_terminate) {
    std::unique_lock lock(m_queue_mutex);
    if (!m_queue.empty()) {
      auto job = m_queue.back();
      m_queue.pop();
      lock.unlock();
      job->Execute();
      job->SetState(JobState::EXECUTION_FINISHED);
    } else {
      m_queue_condition.wait(lock);
      continue;
    }
  }
}

void SingleThreadedExecutionImpl::Start() {
  if (m_current_state == JobExecutionState::STOPPED) {
    m_should_terminate = false;
    m_worker_thread = std::make_unique<std::thread>(
        &SingleThreadedExecutionImpl::ExecuteJobs, this);
    m_current_state = JobExecutionState::RUNNING;
  }
}

void SingleThreadedExecutionImpl::Stop() {
  if (m_current_state == JobExecutionState::RUNNING) {
    m_should_terminate = true;
    m_queue_condition.notify_all();
    m_worker_thread->join();
    m_current_state = JobExecutionState::STOPPED;
  }
}

using namespace std::chrono_literals;
void SingleThreadedExecutionImpl::WaitForCompletion(
    std::shared_ptr<JobCounter> counter) {
  while (!counter->AreAllFinished()) {
    std::this_thread::sleep_for(10ms);
  }
}
