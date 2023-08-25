#include "jobsystem/manager/impl/BasicJobManager.h"
#include "logging/Logging.h"

void jobsystem::manager::impl::BasicJobManager::Init() {
  auto logger = logging::Logging::GetLogger();
  logger->Info("Performing init");
}

void jobsystem::manager::impl::BasicJobManager::Shutdown() {}
void jobsystem::manager::impl::BasicJobManager::InvokeCycle() {}
void jobsystem::manager::impl::BasicJobManager::InvokeCycleAsync() {}
void jobsystem::manager::impl::BasicJobManager::KickJob(
    std::shared_ptr<JobDecl> job_declaration) {}

void jobsystem::manager::impl::BasicJobManager::KickJob(
    std::shared_ptr<JobDecl> job_declaration,
    JobExecutionPhase execution_phase) {}
