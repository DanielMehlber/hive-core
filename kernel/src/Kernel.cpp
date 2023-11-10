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
  //  m_job_manager = std::make_shared<JobManager>();
  //  m_message_broker = MessagingFactory::CreateBroker(m_job_manager);
  //  m_property_provider =
  //  std::make_shared<PropertyProvider>(m_message_broker); m_resource_manager =
  //  ResourceFactory::CreateResourceManager(); m_service_registry =
  //      std::make_shared<services::impl::LocalOnlyServiceRegistry>();
  //
  //  auto plugin_context = std::make_shared<PluginContext>(
  //      m_job_manager, m_property_provider, m_message_broker, m_renderer,
  //      m_service_registry, m_networking_manager, m_resource_manager);
  //
  //  m_plugin_manager = std::make_shared<plugins::BoostPluginManager>(
  //      plugin_context, m_resource_manager);
}
