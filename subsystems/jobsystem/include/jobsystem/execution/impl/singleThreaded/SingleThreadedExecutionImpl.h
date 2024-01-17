#ifndef SINGLETHREADEDEXECUTIONIMPL_H
#define SINGLETHREADEDEXECUTIONIMPL_H

#include "common/config/Configuration.h"
#include "jobsystem/execution/IJobExecution.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>


namespace jobsystem::execution::impl {

/**
 * @brief This implementation of the job execution uses a single worker thread
 * that processes all jobs in its queue.
 * @attention This implementation cannot wait for other jobs from inside another
 * job because this would block the one and only worker thread.
 */
class SingleThreadedExecutionImpl
    : public jobsystem::execution::IJobExecution<SingleThreadedExecutionImpl> {
private:
  std::queue<std::shared_ptr<Job>> m_execution_queue;
  std::mutex m_execution_queue_mutex;

  std::unique_ptr<std::thread> m_worker_thread;
  std::condition_variable m_execution_queue_condition;
  std::atomic_bool m_termination_flag;

  jobsystem::execution::JobExecutionState m_current_state{STOPPED};

  void ExecuteJobs(const std::weak_ptr<JobManager> &manager);

public:
  explicit SingleThreadedExecutionImpl(
      const common::config::SharedConfiguration &config);
  virtual ~SingleThreadedExecutionImpl();
  void Schedule(const std::shared_ptr<Job> &job);
  void WaitForCompletion(const std::shared_ptr<IJobWaitable> &waitable);
  void Start(const std::weak_ptr<JobManager> &manager);
  void Stop();
  jobsystem::execution::JobExecutionState GetState();

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
