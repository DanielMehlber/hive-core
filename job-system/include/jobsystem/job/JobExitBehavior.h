#ifndef JOBEXITBEHAVIOR_H
#define JOBEXITBEHAVIOR_H

namespace jobsystem::job {
/**
 * @brief Behavior, that will be applied as soon as a job has been sucessfully
 * completed.
 */
enum JobContinuation {
  /**
   * @brief The jobsystem will be reschedule or re-queue the job automatically,
   * but instead dispose it. If the job has to be repeated, it must be kicked
   * manually.
   */
  DISPOSE,
  /**
   * @brief The jobsystem will requeue the job automatically after its
   * completion. This is useful for periodic jobs.
   */
  REQUEUE
};
} // namespace jobsystem::job

#endif /* JOBEXITBEHAVIOR_H */
