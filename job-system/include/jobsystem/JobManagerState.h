#ifndef JOBMANAGERSTATE_H
#define JOBMANAGERSTATE_H

namespace jobsystem {

/**
 * @brief Current state of the job manager
 */
enum JobManagerState {
  /**
   * @brief The manager is currently collecting jobs, but no execution cycle has
   * started yet.
   */
  READY,

  /**
   * @brief All jobs assigned to the initialization phase of the cycle are
   * currently executed.
   */
  CYCLE_INIT,

  /**
   * @brief All jobs assigned to the main processing phase of the cycle are
   * currently executed.
   */
  CYCLE_MAIN,

  /**
   * @brief All jobs assigned to the clean-up phase of the cycle are currently
   * executed.
   */
  CYCLE_CLEAN_UP
};
} // namespace jobsystem

#endif /* JOBMANAGERSTATE_H */
