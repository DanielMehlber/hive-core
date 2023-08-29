#ifndef JOBEXECUTIONPHASE_H
#define JOBEXECUTIONPHASE_H

namespace jobsystem::job {
/**
 * @brief The job system has multiple phases of execution for each processing
 * cycle to avoid data races. This can happen, when a resource/component is
 * initialized and used in the same cycle: The resource could be used before
 * initialization, which must be avoided.
 */
enum JobExecutionPhase {
  /**
   * @brief Initialization phase of the cycle. Initialize and prepare stuff in
   * this phase before using it in the main phase.
   */
  INIT,

  /**
   * @brief Main execution phase of the cycle. Do the actual work here.
   */
  MAIN,

  /**
   * @brief Clean-up phase after the main workload has been processed.
   * Shut-down, release and delete stuff here.
   */
  CLEAN_UP
};
} // namespace jobsystem::job

#endif /* JOBEXECUTIONPHASE_H */
