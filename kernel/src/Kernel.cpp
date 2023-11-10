#include "kernel/Kernel.h"
#include "messaging/MessagingFactory.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resourcemgmt/ResourceFactory.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"

using namespace kernel;
using namespace jobsystem;
using namespace messaging;
using namespace props;
using namespace resourcemgmt;
using namespace services;
using namespace plugins;

Kernel::Kernel() {
  auto job_manager = std::make_shared<JobManager>();
  m_subsystems->AddOrReplaceSubsystem(job_manager);

  auto message_broker = MessagingFactory::CreateBroker(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(message_broker);

  auto property_provider = std::make_shared<PropertyProvider>(m_subsystems);
  m_subsystems->AddOrReplaceSubsystem(property_provider);

  auto resource_manager = ResourceFactory::CreateResourceManager();
  m_subsystems->AddOrReplaceSubsystem(resource_manager);

  auto service_registry =
      std::make_shared<services::impl::LocalOnlyServiceRegistry>();
  m_subsystems->AddOrReplaceSubsystem<IServiceRegistry>(service_registry);

  auto plugin_context = std::make_shared<PluginContext>(m_subsystems);
  auto plugin_manager = std::make_shared<plugins::BoostPluginManager>(
      plugin_context, m_subsystems);
  m_subsystems->AddOrReplaceSubsystem<IPluginManager>(plugin_manager);
}
