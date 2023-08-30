#ifndef SINGLETHREADEDEXECUTIONIMPL_H
#define SINGLETHREADEDEXECUTIONIMPL_H

#include "jobsystem/execution/IJobExecution.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

using namespace jobsystem::execution;
using namespace jobsystem::job;

namespace jobsystem::execution::impl {
class SingleThreadedExecutionImpl
    : public IJobExecution<SingleThreadedExecutionImpl> {
private:
  std::queue<std::shared_ptr<Job>> m_execution_queue;
  std::mutex m_execution_queue_mutex;

  std::unique_ptr<std::thread> m_worker_thread;
  std::condition_variable m_execution_queue_condition;
  std::atomic_bool m_termination_flag;

  JobExecutionState m_current_state{STOPPED};

  void ExecuteJobs(JobManager *manager);

public:
  SingleThreadedExecutionImpl();
  ~SingleThreadedExecutionImpl();
  void Schedule(std::shared_ptr<Job> job);
  void WaitForCompletion(std::shared_ptr<JobCounter> counter);
  void Start(JobManager *manager);
  void Stop();
  JobExecutionState GetState();
};

inline JobExecutionState SingleThreadedExecutionImpl::GetState() {
  return m_current_state;
}

} // namespace jobsystem::execution::impl

#endif /* SINGLETHREADEDEXECUTIONIMPL_H */
