#include "core/Core.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "graphics/service/RenderService.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resources/loader/impl/FileLoader.h"
#include "resources/manager/impl/ThreadPoolResourceManager.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include "services/registry/impl/p2p/PeerToPeerServiceRegistry.h"

using namespace hive;
using namespace hive::core;
using namespace hive::jobsystem;
using namespace hive::events;
using namespace hive::data;
using namespace hive::resources;
using namespace hive::services;
using namespace hive::plugins;
using namespace hive::networking;
using namespace hive::graphics;

Core::Core(common::config::SharedConfiguration config, bool only_local)
    : m_subsystems(
          common::memory::Owner<common::subsystems::SubsystemManager>()) {

  // setup job manager subsystem
  auto job_manager = common::memory::Owner<JobManager>(config);
  job_manager->StartExecution();
  m_subsystems->AddOrReplaceSubsystem(std::move(job_manager));

  // setup event broker subsystem
  SetEventBroker(common::memory::Owner<events::brokers::JobBasedEventBroker>(
      m_subsystems.CreateReference()));

  // setup property provider
  SetDataLayer(common::memory::Owner<DataLayer>(m_subsystems.Borrow()));

  // setup resource manager
  auto resource_manager = common::memory::Owner<ThreadPoolResourceManager>();
  auto file_resource_loader = std::make_shared<loaders::FileLoader>();
  resource_manager->RegisterLoader(file_resource_loader);
  SetResourceManager(std::move(resource_manager));

  if (only_local) {
    // only setup local service registry
    SetServiceRegistry(
        common::memory::Owner<services::impl::LocalOnlyServiceRegistry>());
  } else {
    // setup networking system for remote service calls
    SetNetworkingManager(common::memory::Owner<networking::NetworkingManager>(
        m_subsystems.CreateReference(), config));

    // setup remote-capable service registry
    SetServiceRegistry(
        common::memory::Owner<services::impl::PeerToPeerServiceRegistry>(
            m_subsystems.CreateReference()));
  }

  auto plugin_context =
      std::make_shared<PluginContext>(m_subsystems.CreateReference(), config);
  SetPluginManager(common::memory::Owner<plugins::BoostPluginManager>(
      plugin_context, m_subsystems.CreateReference()));
}

Core::~Core() {
  LOG_INFO("shutting down hive core system")

  // plugins must be unloaded before shutting down dependent subsystems
  m_subsystems->RemoveSubsystem<plugins::IPluginManager>();
};

void Core::EnableRenderingJob() {
  auto job_manager = m_subsystems->RequireSubsystem<jobsystem::JobManager>();

  // enabling the renderer is a job: this results in a job-in-job creation
  auto enable_rendering_job_job = std::make_shared<jobsystem::Job>(
      [subsystems = m_subsystems.Borrow()](JobContext *context) mutable {
        auto job_manager =
            subsystems->RequireSubsystem<jobsystem::JobManager>();
        job_manager->DetachJob("render-job");
        auto rendering_job = std::make_shared<jobsystem::TimerJob>(
            [subsystems](jobsystem::JobContext *context) mutable {
              auto maybe_rendering_subsystem =
                  subsystems->GetSubsystem<graphics::IRenderer>();
              if (maybe_rendering_subsystem.has_value()) {
                bool continue_rendering =
                    maybe_rendering_subsystem.value()->Render();
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

void Core::EnableRenderingService(
    common::memory::Reference<IRenderer> service_renderer) {
  common::memory::Reference<IRenderer> renderer;
  if (service_renderer) {
    renderer = service_renderer;
  } else {
    renderer =
        m_subsystems->RequireSubsystem<graphics::IRenderer>().ToReference();
  }
  auto render_service = std::make_shared<graphics::RenderService>(
      m_subsystems.CreateReference(), renderer);
  auto service_registry = m_subsystems->RequireSubsystem<IServiceRegistry>();
  service_registry->Register(render_service);
}

common::memory::Borrower<common::subsystems::SubsystemManager>
Core::GetSubsystemsManager() {
  return m_subsystems.Borrow();
}

bool Core::ShouldShutdown() const { return m_should_shutdown; }

void Core::Shutdown(bool value) { m_should_shutdown = value; }

void Core::SetEventBroker(
    common::memory::Owner<events::IEventBroker> &&broker) {
  m_subsystems->AddOrReplaceSubsystem<IEventBroker>(std::move(broker));
}

common::memory::Borrower<events::IEventBroker> Core::GetEventBroker() {
  return m_subsystems->RequireSubsystem<IEventBroker>();
}

void Core::SetDataLayer(common::memory::Owner<DataLayer> &&broker) {
  m_subsystems->AddOrReplaceSubsystem<DataLayer>(std::move(broker));
}
common::memory::Borrower<DataLayer> Core::GetDataLayer() {
  return m_subsystems->RequireSubsystem<DataLayer>();
}

void Core::SetResourceManager(
    common::memory::Owner<IResourceManager> &&broker) {
  m_subsystems->AddOrReplaceSubsystem<IResourceManager>(std::move(broker));
}

common::memory::Borrower<IResourceManager> Core::GetResourceManager() {
  return m_subsystems->RequireSubsystem<IResourceManager>();
}

void Core::SetServiceRegistry(
    common::memory::Owner<services::IServiceRegistry> &&registry) {
  m_subsystems->AddOrReplaceSubsystem<IServiceRegistry>(std::move(registry));
}

common::memory::Borrower<services::IServiceRegistry>
Core::GetServiceRegistry() {
  return m_subsystems->RequireSubsystem<IServiceRegistry>();
}

void Core::SetNetworkingManager(
    common::memory::Owner<networking::NetworkingManager> &&manager) {
  m_subsystems->AddOrReplaceSubsystem<NetworkingManager>(std::move(manager));
}

common::memory::Borrower<networking::NetworkingManager>
Core::GetNetworkingManager() {
  return m_subsystems->RequireSubsystem<NetworkingManager>();
}

void Core::SetPluginManager(
    common::memory::Owner<plugins::IPluginManager> &&manager) {
  m_subsystems->AddOrReplaceSubsystem<IPluginManager>(std::move(manager));
}

common::memory::Borrower<plugins::IPluginManager> Core::GetPluginManager() {
  return m_subsystems->RequireSubsystem<IPluginManager>();
}

void Core::SetRenderer(common::memory::Owner<graphics::IRenderer> &&renderer) {
  m_subsystems->AddOrReplaceSubsystem<IRenderer>(std::move(renderer));
}

common::memory::Borrower<graphics::IRenderer> Core::GetRenderer() {
  return m_subsystems->RequireSubsystem<IRenderer>();
}

common::memory::Borrower<jobsystem::JobManager> Core::GetJobManager() {
  return m_subsystems->RequireSubsystem<JobManager>();
}
