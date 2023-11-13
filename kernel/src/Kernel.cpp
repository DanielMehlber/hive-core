#include "kernel/Kernel.h"
#include "messaging/MessagingFactory.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resourcemgmt/ResourceFactory.h"
#include "resourcemgmt/loader/impl/FileLoader.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"

using namespace kernel;
using namespace jobsystem;
using namespace messaging;
using namespace props;
using namespace resourcemgmt;
using namespace services;
using namespace plugins;

Kernel::Kernel(common::subsystems::SharedSubsystemManager subsystems)
    : m_subsystems(std::move(subsystems)) {

  auto job_manager = std::make_shared<JobManager>();
  m_subsystems->AddOrReplaceSubsystem(job_manager);

  auto message_broker = MessagingFactory::CreateBroker(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(message_broker);

  auto property_provider = std::make_shared<PropertyProvider>(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(property_provider);

  auto resource_manager = ResourceFactory::CreateResourceManager();
  m_subsystems->AddOrReplaceSubsystem(resource_manager);
  auto file_resource_loader =
      std::make_shared<resourcemgmt::loaders::FileLoader>();
  resource_manager->RegisterLoader(file_resource_loader);

  auto service_registry =
      std::make_shared<services::impl::LocalOnlyServiceRegistry>();
  m_subsystems->AddOrReplaceSubsystem<IServiceRegistry>(service_registry);

  auto plugin_context = std::make_shared<PluginContext>(m_subsystems);
  auto plugin_manager = std::make_shared<plugins::BoostPluginManager>(
      plugin_context, m_subsystems);
  m_subsystems->AddOrReplaceSubsystem<IPluginManager>(plugin_manager);
}

void Kernel::EnableRenderingJob() {
  auto job_manager = m_subsystems->RequireSubsystem<jobsystem::JobManager>();

  // enabling the renderer is a job: this results in a job-in-job creation
  auto enable_rendering_job_job = std::make_shared<jobsystem::job::Job>(
      [subsystems = m_subsystems](JobContext *context) {
        auto job_manager =
            subsystems->RequireSubsystem<jobsystem::JobManager>();
        job_manager->DetachJob("render-job");
        auto rendering_job = std::make_shared<jobsystem::job::TimerJob>(
            [subsystems](jobsystem::JobContext *context) {
              auto opt_rendering_subsystem =
                  subsystems->GetSubsystem<graphics::IRenderer>();
              if (opt_rendering_subsystem.has_value()) {
                bool continue_rendering =
                    opt_rendering_subsystem.value()->Render();
                if (continue_rendering) {
                  return JobContinuation::REQUEUE;
                } else {
                  return JobContinuation::DISPOSE;
                }
              } else {
                LOG_WARN(
                    "Cannot continue rendering job because rendering subsystem "
                    "was removed")
                return JobContinuation::DISPOSE;
              }
            },
            "render-job", 16ms, MAIN);
        job_manager->KickJob(rendering_job);
        return JobContinuation::DISPOSE;
      },
      "enable-rendering-job", INIT);

  job_manager->KickJob(enable_rendering_job_job);
}
