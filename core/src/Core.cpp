#include "core/Core.h"
#include "events/EventFactory.h"
#include "graphics/service/RenderService.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resourcemgmt/ResourceFactory.h"
#include "resourcemgmt/loader/impl/FileLoader.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"

using namespace kernel;
using namespace jobsystem;
using namespace events;
using namespace props;
using namespace resourcemgmt;
using namespace services;
using namespace plugins;

Core::Core(common::config::SharedConfiguration config, bool only_local)
    : m_subsystems(std::make_shared<common::subsystems::SubsystemManager>()) {

  auto job_manager = std::make_shared<JobManager>(config);
  m_subsystems->AddOrReplaceSubsystem(job_manager);
  job_manager->StartExecution();

  auto message_broker = EventFactory::CreateBroker(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(message_broker);

  auto property_provider = std::make_shared<PropertyProvider>(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(property_provider);

  auto resource_manager = ResourceFactory::CreateResourceManager();
  m_subsystems->AddOrReplaceSubsystem(resource_manager);
  auto file_resource_loader =
      std::make_shared<resourcemgmt::loaders::FileLoader>();
  resource_manager->RegisterLoader(file_resource_loader);

  if (only_local) {
    auto service_registry =
        std::make_shared<services::impl::LocalOnlyServiceRegistry>();
    m_subsystems->AddOrReplaceSubsystem<IServiceRegistry>(service_registry);
  } else {
    auto network_manager =
        std::make_shared<networking::NetworkingManager>(m_subsystems, config);
    m_subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
        network_manager);

    auto service_registry =
        std::make_shared<services::impl::RemoteServiceRegistry>(m_subsystems);
    m_subsystems->AddOrReplaceSubsystem<IServiceRegistry>(service_registry);
  }

  auto plugin_context = std::make_shared<PluginContext>(m_subsystems);
  auto plugin_manager = std::make_shared<plugins::BoostPluginManager>(
      plugin_context, m_subsystems);
  m_subsystems->AddOrReplaceSubsystem<IPluginManager>(plugin_manager);
}

Core::~Core() {}

void Core::EnableRenderingJob() {
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

void Core::EnableRenderingService(graphics::SharedRenderer serivce_renderer) {
  graphics::SharedRenderer renderer;
  if (serivce_renderer) {
    renderer = serivce_renderer;
  } else {
    renderer = m_subsystems->RequireSubsystem<graphics::IRenderer>();
  }
  auto render_service =
      std::make_shared<graphics::RenderService>(m_subsystems, renderer);
  auto service_registry = m_subsystems->RequireSubsystem<IServiceRegistry>();
  service_registry->Register(render_service);
}

common::subsystems::SharedSubsystemManager Core::GetSubsystemsManager() const {
  return m_subsystems;
}

jobsystem::SharedJobManager Core::GetJobManager() const {
  return m_subsystems->GetSubsystem<jobsystem::JobManager>().value();
}

props::SharedPropertyProvider Core::GetPropertyProvider() const {
  return m_subsystems->GetSubsystem<props::PropertyProvider>().value();
}

resourcemgmt::SharedResourceManager Core::GetResourceManager() const {
  return m_subsystems->GetSubsystem<resourcemgmt::IResourceManager>().value();
}

events::SharedEventBroker Core::GetMessageBroker() const {
  return m_subsystems->GetSubsystem<events::IEventBroker>().value();
}

void Core::SetResourceManager(
    const resourcemgmt::SharedResourceManager &resourceManager) {
  m_subsystems->AddOrReplaceSubsystem<resourcemgmt::IResourceManager>(
      resourceManager);
}

services::SharedServiceRegistry Core::GetServiceRegistry() const {
  return m_subsystems->GetSubsystem<services::IServiceRegistry>().value();
}

void Core::SetServiceRegistry(
    const services::SharedServiceRegistry &serviceRegistry) {
  m_subsystems->GetSubsystem<services::IServiceRegistry>().value();
}

networking::SharedNetworkingManager Core::GetNetworkingManager() const {
  return m_subsystems->GetSubsystem<networking::NetworkingManager>().value();
}

void Core::SetNetworkingManager(
    const networking::SharedNetworkingManager &networkingManager) {
  m_subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
      networkingManager);
}

plugins::SharedPluginManager Core::GetPluginManager() const {
  return m_subsystems->GetSubsystem<plugins::IPluginManager>().value();
}

void Core::SetPluginManager(const plugins::SharedPluginManager &pluginManager) {
  m_subsystems->AddOrReplaceSubsystem<plugins::IPluginManager>(pluginManager);
}

scene::SharedScene Core::GetScene() const {
  return m_subsystems->GetSubsystem<scene::SceneManager>().value();
}

void Core::SetScene(const scene::SharedScene &scene) {
  m_subsystems->AddOrReplaceSubsystem<scene::SceneManager>(scene);
}

graphics::SharedRenderer Core::GetRenderer() const {
  return m_subsystems->GetSubsystem<graphics::IRenderer>().value();
}

void Core::SetRenderer(const graphics::SharedRenderer &renderer) {
  m_subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(renderer);
}

bool Core::ShouldShutdown() const { return m_should_shutdown; }

void Core::Shutdown(bool value) { m_should_shutdown = value; }
