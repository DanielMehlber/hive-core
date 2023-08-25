#ifndef BASICJOBMANAGER_H
#define BASICJOBMANAGER_H

#include <queue>

#include "../IJobManager.h"

namespace jobsystem::manager::impl {
class BasicJobManager
    : public jobsystem::manager::IJobManager<BasicJobManager> {
protected:
  std::queue<std::shared_ptr<jobsystem::JobDecl>> m_init_cycle_jobs;
  std::queue<std::shared_ptr<jobsystem::JobDecl>> m_main_cycle_jobs;
  std::queue<std::shared_ptr<jobsystem::JobDecl>> m_exit_cycle_jobs;

public:
  void Init();
  void Shutdown();
  void InvokeCycle();
  void InvokeCycleAsync();
  void KickJob(std::shared_ptr<jobsystem::JobDecl> job_declaration);
  void KickJob(std::shared_ptr<jobsystem::JobDecl> job_declaration,
               jobsystem::manager::JobExecutionPhase execution_phase);
};
} // namespace jobsystem::manager::impl

#endif /* BASICJOBMANAGER_H */
