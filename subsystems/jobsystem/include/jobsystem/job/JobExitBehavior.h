#ifndef JOBEXITBEHAVIOR_H
#define JOBEXITBEHAVIOR_H

namespace jobsystem::job {
/**
 * @brief Behavior, that will be applied as soon as a job has been sucessfully
 * completed.
 */
enum JobContinuation {
  /**
   * @brief The jobsystem will not reschedule or re-queue the job after
   * execution, but instead dispose it.
   * @note If the job has to be repeated, it must be kicked manually.
   */
  DISPOSE,
  /**
   * @brief The jobsystem will requeue the job automatically after its
   * completion.
   * @note This is useful for periodic jobs.
   */
  REQUEUE
};
} // namespace jobsystem::job

#endif /* JOBEXITBEHAVIOR_H */
