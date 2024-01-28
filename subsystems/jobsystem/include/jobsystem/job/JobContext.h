#ifndef JOBCONTEXT_H
#define JOBCONTEXT_H

#include <chrono>
#include <memory>
#include <utility>

namespace jobsystem {

// Forward declaration
class JobManager;

/**
 * Contains data that could be relevant during the job execution cycle
 * and enables running jobs to communicate with the job manager.
 */
class JobContext {
protected:
  size_t m_cycle_number;
  std::weak_ptr<JobManager> m_job_manager;

public:
  JobContext(size_t frame_number, const std::shared_ptr<JobManager> &manager)
      : m_cycle_number{frame_number}, m_job_manager{manager} {}

  /**
   * GetAsInt number of current job cycle
   * @return current job cycle number
   */
  size_t GetCycleNumber() const noexcept;

  /**
   * GetAsInt the managing instance of the current job execution
   * @return managing instance of the current job execution
   */
  std::shared_ptr<JobManager> GetJobManager() noexcept;
};

inline size_t jobsystem::JobContext::GetCycleNumber() const noexcept {
  return m_cycle_number;
}

inline std::shared_ptr<JobManager> JobContext::GetJobManager() noexcept {
  return m_job_manager.lock();
}

} // namespace jobsystem

#endif /* JOBCONTEXT_H */
