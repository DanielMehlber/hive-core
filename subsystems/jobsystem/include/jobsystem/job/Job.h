#ifndef JOB_H
#define JOB_H

#include "JobContext.h"
#include "JobExecutionPhase.h"
#include "JobExitBehavior.h"
#include "JobState.h"
#include "jobsystem/synchronization/JobCounter.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace jobsystem::job {

/**
 * A job is the central data structure of the job system. It contains
 * work that must be executed for the job to complete and metadata used in this
 * process.
 */
class Job {
protected:
  /** ID of this job */
  const std::string m_id;

  /**
   * Workload encapsulated in a function that will be executed by the job
   * execution.
   */
  std::function<JobContinuation(JobContext *)> m_workload;

  /** Tracks progress and current state of this job. */
  JobState m_current_state{DETACHED};

  /** Workload will be executed in the given phase of the execution cycle. */
  JobExecutionPhase m_phase;

  /** All counters that track the progress of this job. */
  std::queue<std::shared_ptr<JobCounter>> m_counters;
  mutable mutex m_counters_mutex;

public:
  Job(std::function<JobContinuation(JobContext *)>, std::string id,
      JobExecutionPhase phase = MAIN);
  explicit Job(std::function<JobContinuation(JobContext *)>,
               JobExecutionPhase phase = MAIN);
  virtual ~Job() { FinishJob(); };

  /**
   * Notifies all counters that this job has finished and removes them
   * from the counters list.
   */
  void FinishJob();

  /**
   * Executes the workload in the calling thread and adjusts state variables of
   * this job.
   */
  JobContinuation Execute(JobContext *);

  /**
   * Checks if this job can be scheduled (typically in the next cycle)
   * considering the current context. If not, this job should be skipped and
   * asked again before the next cycle.
   * @param context context information that may be relevant for the scheduling
   * decision.
   * @return true, if this job can be scheduled in the upcoming cycle.
   */
  virtual bool IsReadyForExecution(const JobContext &context) { return true; };

  /**
   * Adds counter for this job to the counter list, incrementing the counter.
   * @param counter additional counter
   */
  void AddCounter(const std::shared_ptr<JobCounter> &counter);

  JobState GetState();
  void SetState(JobState state);
  JobExecutionPhase GetPhase();
  const std::string &GetId();
};

inline JobState Job::GetState() { return m_current_state; }
inline void Job::SetState(JobState state) { m_current_state = state; }
inline JobExecutionPhase Job::GetPhase() { return m_phase; }
inline const std::string &Job::GetId() { return m_id; }

typedef std::shared_ptr<jobsystem::job::Job> SharedJob;

} // namespace jobsystem::job

#endif /* JOB_H */
