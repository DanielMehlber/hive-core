#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include "JobManagerState.h"
#include "jobsystem/execution/IJobExecution.h"
#include "jobsystem/execution/impl/singleThreaded/SingleThreadedExecutionImpl.h"
#include "jobsystem/job/Job.h"
#include "jobsystem/job/TimerJob.h"
#include "logging/Logging.h"
#include <memory>
#include <queue>

using namespace jobsystem::job;
using namespace jobsystem::execution;

typedef impl::SingleThreadedExecutionImpl JobExecutionImpl;

namespace jobsystem {
class JobManager {
private:
  /**
   * @brief All jobs for the initialization phase of the cycle are collected
   * here.
   */
  std::queue<std::shared_ptr<Job>> m_init_queue;
  std::mutex m_init_queue_mutex;

  /**
   * @brief All jobs for the main processing phase of the cycle are collected
   * here.
   */
  std::queue<std::shared_ptr<Job>> m_main_queue;
  std::mutex m_main_queue_mutex;

  /**
   * @brief All jobs for the clean-up phase of the cycle are collected here.
   */
  std::queue<std::shared_ptr<Job>> m_clean_up_queue;
  std::mutex m_clean_up_queue_mutex;

  /**
   * @brief All jobs that should not be kicked for the following cycles instead
   * for the currently running cycles will be collected here. This is the case,
   * when a job is automatically rescheduled for future cycles (e.g.
   * time-interval jobs).
   */
  std::queue<std::shared_ptr<Job>> m_next_cycle_queue;
  std::mutex m_next_cycle_queue_mutex;

  JobManagerState m_current_state{READY};
  JobExecutionImpl m_execution;

  size_t m_cycle_count{0};

  void ExecutePhaseAndWait(std::queue<std::shared_ptr<Job>> &queue,
                           std::mutex &queue_mutex);

public:
  void KickJob(std::shared_ptr<Job> job);
  void KickJobForNextCycle(std::shared_ptr<Job> job);
  void InvokeCycleAndWait();
  void WaitForCompletion(std::shared_ptr<JobCounter> counter);
  size_t GetCycleCount() noexcept;
};

inline void JobManager::WaitForCompletion(std::shared_ptr<JobCounter> counter) {
  m_execution.WaitForCompletion(counter);
}

} // namespace jobsystem

#endif /* JOBMANAGER_H */
