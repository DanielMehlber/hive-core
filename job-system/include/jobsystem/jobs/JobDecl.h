#ifndef JOBDECL_H
#define JOBDECL_H

#include <functional>
#include <memory>
#include <string>

#include "jobsystem/context/JobContext.h"

namespace jobsystem::jobs {

enum JobDeclState { DETACHED, QUEUED, IN_EXECUTION, FINISHED };

/**
 * Declares a job that should be scheduled in the next job execution cycles or
 * in the near future.
 */
class JobDecl {
protected:
  std::string m_name;
  const std::function<void()> m_execution;
  JobDeclState m_current_job_state;

public:
  JobDecl() = delete;
  JobDecl(std::function<void()> execution);
  JobDecl(std::function<void()> execution, std::string name);
  virtual ~JobDecl();

  inline JobDeclState GetState() noexcept;
  inline void SetState(JobDeclState state) noexcept;
  inline std::string GetName() noexcept;

  /**
   * @brief Checks if the job declaration should be execution in the current job
   * execution cycle or at some later point in time. This can be used to delay
   * the exeution.
   * @param context context of the job system containing relevant data for the
   * decision
   * @return true if the job declaration should be executed in the current job
   * cycle
   */
  virtual bool IsReady(const JobContext &context) noexcept;

  /**
   * @brief Determines what behavior should apply after the instance of this
   * job has finished: The declaration can be disposed or automatically
   * re-queued for future processing (i.e. useful for jobs executing in fixed
   * intervals)
   * @param context context of the job system containing dicision-relevant data
   * @return true if the job declaration should be disposed after its instances
   * have finished
   */
  virtual bool IsDisposableAfterExecution(const JobContext &context) noexcept;
};

} // namespace jobsystem::jobs

jobsystem::jobs::JobDecl::JobDecl(std::function<void()> execution)
    : m_execution{execution} {
  static u_int64_t last_job_number;
  last_job_number++;

  m_name = "job-" + std::to_string(last_job_number);
}

jobsystem::jobs::JobDecl::JobDecl(std::function<void()> execution,
                                  std::string name)
    : m_execution{execution}, m_name{name} {}

jobsystem::jobs::JobDecl::~JobDecl() {}

inline jobsystem::jobs::JobDeclState
jobsystem::jobs::JobDecl::GetState() noexcept {
  return m_current_job_state;
}

inline void jobsystem::jobs::JobDecl::SetState(JobDeclState state) noexcept {
  m_current_job_state = state;
}

inline std::string jobsystem::jobs::JobDecl::GetName() noexcept {
  return m_name;
}

inline bool
jobsystem::jobs::JobDecl::IsReady(const JobContext &context) noexcept {
  return true;
}

inline bool jobsystem::jobs::JobDecl::IsDisposableAfterExecution(
    const JobContext &context) noexcept {
  return true;
}

#endif /* JOBDECL */
