#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include <chrono>
#include <memory>

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
  common::memory::Reference<JobManager> m_job_manager;

public:
  JobContext(size_t frame_number, common::memory::Borrower<JobManager> manager)
      : m_cycle_number{frame_number}, m_job_manager{manager.ToReference()} {}

  /**
   * GetAsInt number of current job cycle
   * @return current job cycle number
   */
  size_t GetCycleNumber() const;

  /**
   * GetAsInt the managing instance of the current job execution
   * @return managing instance of the current job execution
   */
  common::memory::Borrower<JobManager> GetJobManager();
};

inline size_t jobsystem::JobContext::GetCycleNumber() const {
  return m_cycle_number;
}

inline common::memory::Borrower<JobManager> JobContext::GetJobManager() {
  return m_job_manager.Borrow();
}

} // namespace jobsystem
