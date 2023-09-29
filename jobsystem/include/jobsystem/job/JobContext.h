#ifndef JOBCONTEXT_H
#define JOBCONTEXT_H

#include <chrono>

namespace jobsystem {

// Forward declaration
class JobManager;

/**
 * @brief Contains data that could be relevant during the job execution cycle
 * and enables running jobs to communicate with the job manager.
 */
class JobContext {
protected:
  size_t m_cycle_number;
  JobManager *m_job_manager;

public:
  JobContext(size_t frame_number, JobManager *manager)
      : m_cycle_number{frame_number}, m_job_manager{manager} {}

  /**
   * @brief Get number of current job cycle
   * @return current job cycle number
   */
  size_t GetCycleNumber() noexcept;

  /**
   * @brief Get the managing instance of the current job execution
   * @return managing instance of the current job execution
   */
  JobManager *GetJobManager() noexcept;
};

inline size_t jobsystem::JobContext::GetCycleNumber() noexcept {
  return m_cycle_number;
}

inline JobManager *JobContext::GetJobManager() noexcept {
  return m_job_manager;
}

} // namespace jobsystem

#endif /* JOBCONTEXT_H */
