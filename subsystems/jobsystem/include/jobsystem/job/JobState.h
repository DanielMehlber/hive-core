#ifndef JOBSTATE_H
#define JOBSTATE_H

namespace jobsystem::job {

/**
 * Current state of a job instance.
 */
enum JobState {
  /**
   * The job is not managed by any job system. This can be the
   * case, if the job has been successfully executed and was therefore disposed
   * by the job system.
   */
  DETACHED,

  /**
   * The job is managed by some job system and currently waits for execution
   * inside a queue. This is the case after it has been kicked but hasn't been
   * executed yet.
   */
  QUEUED,

  /**
   * The job instance has been passed to the job execution and will be
   * processed shortly.
   */
  AWAITING_EXECUTION,

  /**
   * The job is currently executing its workload. It could also be in a waiting
   * state.
   */
  IN_EXECUTION,

  /**
   * The job has been executed successfully.
   */
  EXECUTION_FINISHED
};
} // namespace jobsystem::job

#endif /* JOBSTATE_H */
