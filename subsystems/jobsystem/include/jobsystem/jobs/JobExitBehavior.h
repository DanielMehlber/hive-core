#pragma once

namespace hive::jobsystem {

/**
 * Behavior, that will be applied as soon as a job has been successfully
 * completed. It basically describes how to continue after the job has been
 * processed, hence the name.
 */
enum JobContinuation {
  /**
   * The job system will not reschedule or re-queue the job after
   * execution, but instead dispose it.
   * @note If the job has to be repeated, it must be kicked again by the user.
   */
  DISPOSE,

  /**
   * The job system will requeue the job automatically after its
   * completion. It will be executed once it is ready again.
   * @note This is useful for periodic jobs.
   */
  REQUEUE
};
} // namespace hive::jobsystem
