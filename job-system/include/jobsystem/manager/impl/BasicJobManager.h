#ifndef BASICJOBMANAGER_H
#define BASICJOBMANAGER_H

#include <queue>

#include "jobsystem/manager/IJobManager.h"

namespace jobsystem::manager::impl {
class BasicJobManager
    : public jobsystem::manager::IJobManager<BasicJobManager> {
protected:
  std::queue<std::shared_ptr<jobsystem::jobs::JobDecl>> m_init_cycle_jobs;
  std::queue<std::shared_ptr<jobsystem::jobs::JobDecl>> m_main_cycle_jobs;
  std::queue<std::shared_ptr<jobsystem::jobs::JobDecl>> m_exit_cycle_jobs;

public:
  void Init();
  void Shutdown();
  void InvokeCycle();
  void InvokeCycleAsync();
  void KickJob(
      jobsystem::jobs::JobDecl job_declaration,
      jobsystem::manager::JobExecutionPhase execution_phase = CYCLE_MAIN_PHASE);
};
} // namespace jobsystem::manager::impl

#endif /* BASICJOBMANAGER_H */
