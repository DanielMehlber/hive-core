#ifndef JOBSTATE_H
#define JOBSTATE_H

namespace jobsystem::job {
/**
 * @brief Current state of a job instance.
 */
enum JobState {
  /**
   * @brief The job instance is detached from any job system and therfore
   * unmanaged. It must be passed to the job system to be used. This can be the
   * case, if the job has been sucessfully executed and was therefore disposed
   * by the job system.
   */
  DETACHED,

  /**
   * @brief The job instance is handled by the job system and in the job queue,
   * but is not yet processed by the job execution. This is the case, if the job
   * has been passed to the job manager, but it is not included in the current
   * cycle or it hasn't been started yet.
   */
  QUEUED,

  /**
   * @brief The job instance has been passed to the job execution and will be
   * processed every moment.
   */
  AWAITING_EXECUTION,

  /**
   * @brief The job is currently beeing executed by the job execution.
   */
  IN_EXECUTION,

  /**
   * @brief The job has been executed.
   */
  EXECUTION_FINISHED
};
} // namespace jobsystem::job

#endif /* JOBSTATE_H */
