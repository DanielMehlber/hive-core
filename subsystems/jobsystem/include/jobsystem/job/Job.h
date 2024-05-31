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

  /** Some jobs block the current execution cycle until they are finished
   * (synchronized jobs). Others take longer to resolve and therefore must not
   * block the execution cycle. They can finish anytime in the future
   * (asynchronous). */
  bool m_async{false};

  /** All counters that track the progress of this job. */
  std::queue<std::shared_ptr<JobCounter>> m_counters;
  mutable mutex m_counters_mutex;

public:
  /**
   * Creates a new job with a given workload, id, phase and synchronization
   * behavior.
   * @param workload function that will be executed by the job.
   * @param id unique identifier of this job.
   * @param phase in which the job should be executed during the execution
   * cycle.
   * @param async if the job is asynchronous, the cycle will not wait for it to
   * finish.
   */
  Job(std::function<JobContinuation(JobContext *)>, std::string id,
      JobExecutionPhase phase = MAIN, bool async = false);
  
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

  /**
   * Get the current state of this job during the execution cycle. It tracks the
   * current progress of the job.
   * @return Current state of this job.
   */
  JobState GetState();

  /**
   * Set the current state of this job during the execution cycle.
   * @param state new state of this job.
   */
  void SetState(JobState state);

  /**
   * Get phase in which the job should be executed during the execution cycle.
   * @return Phase of execution.
   */
  JobExecutionPhase GetPhase();

  /**
   * Get the unique ID of this job.
   * @return Id of this job.
   */
  const std::string &GetId();

  /**
   * if a job is synchronized with the cycle, the cycle will wait for the job to
   * finish. Asynchronous jobs can finish anytime in the future and are not
   * blocking the cycle.
   * @return true, if job is asynchronous.
   */
  bool IsAsync() const;
};

inline JobState Job::GetState() { return m_current_state; }
inline void Job::SetState(JobState state) { m_current_state = state; }
inline JobExecutionPhase Job::GetPhase() { return m_phase; }
inline const std::string &Job::GetId() { return m_id; }

inline bool Job::IsAsync() const { return m_async; }

typedef std::shared_ptr<jobsystem::job::Job> SharedJob;

} // namespace jobsystem::job

#endif /* JOB_H */
