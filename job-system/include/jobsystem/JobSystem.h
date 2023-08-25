#ifndef JOBSYSTEM_H
#define JOBSYSTEM_H

#include "jobsystem/jobs/JobDecl.h"
#include "jobsystem/manager/IJobManager.h"
#include "jobsystem/manager/impl/BasicJobManager.h"

using namespace jobsystem::manager;
using namespace jobsystem::jobs;

typedef jobsystem::manager::impl::BasicJobManager JobManagerImpl;

namespace jobsystem {

/**
 * API for all operations on the Job System
 */
class JobSystem {
private:
  std::unique_ptr<IJobManager<JobManagerImpl>> m_job_manager;

  inline void Init();
  inline void ShutDown();

  inline void KickJob(JobDecl job_declaration,
                      JobExecutionPhase phase = CYCLE_MAIN_PHASE);

public:
  JobSystem();
  ~JobSystem();
};

inline void jobsystem::JobSystem::Init() {
  m_job_manager = std::make_unique<IJobManager<JobManagerImpl>>();
}
inline void JobSystem::ShutDown() {}

inline void JobSystem::KickJob(JobDecl job_declaration,
                               JobExecutionPhase phase) {
  m_job_manager->KickJob(job_declaration, phase);
}

inline JobSystem::JobSystem() { Init(); }
inline JobSystem::~JobSystem() { ShutDown(); }

} // namespace jobsystem

#endif /* JOBSYSTEM_H */
