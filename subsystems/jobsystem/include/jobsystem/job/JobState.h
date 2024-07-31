#pragma once

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
   * If a job has been queued, but is not ready for execution yet, it will be
   * queued for an upcoming cycle and excluded from the current one.
   */
  RESERVED_FOR_NEXT_CYCLE,

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
  EXECUTION_FINISHED,

  /**
   * The job was not able to complete successfully.
   * @note This can happen if an exception is thrown in the jobs workload.
   */
  FAILED
};
} // namespace jobsystem::job
