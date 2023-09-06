#ifndef JOB_H
#define JOB_H

#include "JobContext.h"
#include "JobCounter.h"
#include "JobExecutionPhase.h"
#include "JobExitBehavior.h"
#include "JobState.h"
#include <functional>
#include <list>
#include <memory>
#include <string>

namespace jobsystem::job {

/**
 * @brief A job is the central data structure of the job system. It contains
 * work that must be executed for the job to complete and metadata used in this
 * process.
 */
class Job {
protected:
  /**
   * @brief Name of this job
   */
  std::string m_name;
  /**
   * @brief Workload encapsulated in a function that will be executed by the job
   * execution.
   */
  std::function<JobContinuation(JobContext *)> m_workload;

  /**
   * @brief Tracks progress and current state of this job
   */
  JobState m_current_state{DETACHED};

  /**
   * @brief The workload will be executed in the given phase of the execution
   * cycle.
   */
  JobExecutionPhase m_phase;

  /**
   * @brief All counters that track the progress of this job.
   */
  std::list<std::shared_ptr<JobCounter>> m_counters;

  /**
   * @brief Notifies all counters that this job has finished and removes them
   * from the counters list.
   */
  void FinishJob();

public:
  Job(std::function<JobContinuation(JobContext *)>, std::string name,
      JobExecutionPhase phase = MAIN);
  Job(std::function<JobContinuation(JobContext *)>,
      JobExecutionPhase phase = MAIN);
  virtual ~Job() { FinishJob(); };

  /**
   * @brief Executes the jobs workload in the calling thread.
   */
  JobContinuation Execute(JobContext *);

  /**
   * @brief Checks if this job can be scheduled (typically in the next cycle)
   * considering the current context. If not, this job should be skipped and
   * asked again before the next cycle.
   * @param context context information that may be relevant for the scheduling
   * decision.
   * @return true, if this job can be scheduled in the upcoming cycle.
   */
  virtual bool IsReadyForExecution(const JobContext &context) { return true; };

  /**
   * @brief Adds counter for this job to the counter list.
   * @param counter additional counter
   */
  void AddCounter(std::shared_ptr<JobCounter> counter);

  JobState GetState() noexcept;
  void SetState(JobState state) noexcept;
  JobExecutionPhase GetPhase() noexcept;
  const std::string &GetName() noexcept;
};

inline JobState Job::GetState() noexcept { return m_current_state; }
inline void Job::SetState(JobState state) noexcept { m_current_state = state; }
inline JobExecutionPhase Job::GetPhase() noexcept { return m_phase; }
inline const std::string &Job::GetName() noexcept { return m_name; }

} // namespace jobsystem::job

#endif /* JOB_H */
