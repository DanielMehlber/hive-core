#pragma once

namespace hive::jobsystem::job {

/**
 * The job system has multiple phases of execution for each processing
 * cycle to avoid data races of resources. This can happen, when a
 * resource/component is initialized and used in the same cycle: The resource
 * could be used before initialization, which must be avoided.
 */
enum JobExecutionPhase {
  /**
   * Initialization phase of the cycle. Initialize and prepare stuff in
   * this phase before using it in the main phase.
   */
  INIT,

  /**
   * Main execution phase of the cycle. Do the actual work here.
   */
  MAIN,

  /**
   * Clean-up phase after the main workload has been processed.
   * Shut-down, release and delete stuff here.
   */
  CLEAN_UP
};
} // namespace hive::jobsystem::job
