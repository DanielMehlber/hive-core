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

/**
 * @brief This implementation of the job execution uses a single worker thread
 * that processes all jobs in its queue.
 * @attention This implementation cannot wait for other jobs from inside another
 * job because this would block the one and only worker thread.
 */
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
  virtual ~SingleThreadedExecutionImpl();
  void Schedule(std::shared_ptr<Job> job);
  void WaitForCompletion(std::shared_ptr<IJobWaitable> waitable);
  void Start(JobManager *manager);
  void Stop();
  JobExecutionState GetState();

  template <typename FutureType>
  void WaitForCompletion(const std::future<FutureType> &future);
};

template <typename FutureType>
inline void SingleThreadedExecutionImpl::WaitForCompletion(
    const std::future<FutureType> &future) {
  future.wait();
}

inline JobExecutionState SingleThreadedExecutionImpl::GetState() {
  return m_current_state;
}

} // namespace jobsystem::execution::impl

#endif /* SINGLETHREADEDEXECUTIONIMPL_H */
