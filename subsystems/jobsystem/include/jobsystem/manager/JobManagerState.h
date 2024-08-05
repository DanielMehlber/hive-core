#pragma once

namespace hive::jobsystem {

/**
 * Current state of the job manager
 */
enum JobManagerState {
  /**
   * The manager is currently collecting jobs, but no execution cycle has
   * started yet.
   */
  READY,

  /**
   * All jobs assigned to the initialization phase of the cycle are
   * currently executed.
   */
  CYCLE_INIT,

  /**
   * All jobs assigned to the main processing phase of the cycle are
   * currently executed.
   */
  CYCLE_MAIN,

  /**
   * All jobs assigned to the clean-up phase of the cycle are currently
   * executed.
   */
  CYCLE_CLEAN_UP
};
} // namespace hive::jobsystem
