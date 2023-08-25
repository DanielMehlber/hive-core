#ifndef IJOBMANAGER_H
#define IJOBMANAGER_H

#include "jobsystem/jobs/JobDecl.h"

using namespace jobsystem::jobs;

namespace jobsystem::manager {

/**
 * A cycle in the job system has multiple execution phases to avoid conflicts
 * and race conditions during an execution cycle. Example: If there is some
 * resource that is initialized by a job and accessed/used by another job in the
 * same cycle, race conditions can occur and it could potentially be used before
 * initalization (ouch). Same goes for usage and clean up of resources.
 */
enum JobExecutionPhase {
  CYCLE_INIT_PHASE,
  CYCLE_MAIN_PHASE,
  CYCLE_CLEAN_UP_PHASE
};

enum JobManagerState { IDLE, INIT_CYCLE, MID_CYCLE, CLEAN_UP_CYCLE };

namespace impl {
class BasicJobManager;
};

/**
 * The JobManager interface declares all functions that must be implemented in
 * order to handle job declarations and execute them. The implementation is done
 * using the Curiously Recurring Template Pattern (CRTP).
 */
template <typename Impl> class IJobManager {
protected:
  JobManagerState m_current_state;

public:
  /**
   * @brief Initializes and prepares the job manager
   */
  void Init();

  /**
   * @brief Clean up any resources the job manager used
   */
  void ShutDown();

  /**
   * @brief Starts new execution cycle and blocks the calling thread until the
   * cycle has finished. This executes all job declarations that have been
   * collected and kicked in the meantime.
   */
  void InvokeCycle();

  /**
   * @brief Starts new execution cycle, but does not block the thread. This
   * executes all job declarations that have been collected and kicked in the
   * meantime.
   */
  void InvokeCycleAsync();

  /**
   * @brief Places job declaration for execution in the current cycle (or the
   * next one, if no cycle is currently running). It will be placed in the
   * default execution phase.
   *
   * @param job_declaration declaration of the job that will be executed
   */
  void KickJob(std::shared_ptr<JobDecl> job_declaration);

  /**
   * @brief Enqueues the job declaration for execution in a specific phase in
   * the current execution cycle (or next one, if none is currently running).
   *
   * @param job_declaration declaration of the job that will be executed
   * @param execution_phase
   */
  void KickJob(std::shared_ptr<JobDecl> job_declaration,
               JobExecutionPhase execution_phase);

  inline JobManagerState GetState() noexcept;
};

template <typename Impl> inline void IJobManager<Impl>::Init() {
  static_cast<Impl *>(this)->Init();
}

template <typename Impl> inline void IJobManager<Impl>::ShutDown() {
  static_cast<Impl *>(this)->ShutDown();
}

template <typename Impl> inline void IJobManager<Impl>::InvokeCycle() {
  static_cast<Impl *>(this)->InvokeCycle();
}

template <typename Impl> inline void IJobManager<Impl>::InvokeCycleAsync() {
  static_cast<Impl *>(this)->InvokeCycleAsync();
}

template <typename Impl>
inline void
IJobManager<Impl>::KickJob(std::shared_ptr<JobDecl> job_declaration) {
  static_cast<Impl *>(this)->KickJob(job_declaration);
}

template <typename Impl>
inline void IJobManager<Impl>::KickJob(std::shared_ptr<JobDecl> job_declaration,
                                       JobExecutionPhase execution_phase) {
  static_cast<Impl *>(this)->KickJob(job_declaration, execution_phase);
}

template <typename Impl>
inline JobManagerState IJobManager<Impl>::GetState() noexcept {
  return m_current_state;
}
} // namespace jobsystem::manager

#endif /* IJOBMANAGER_H */
