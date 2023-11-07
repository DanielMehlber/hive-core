#ifndef PLUGINCONTEXT_H
#define PLUGINCONTEXT_H

#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "messaging/broker/IMessageBroker.h"
#include "networking/NetworkingManager.h"
#include "properties/PropertyProvider.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "services/registry/IServiceRegistry.h"
#include <memory>
#include <utility>

namespace plugins {

class PluginContext {
protected:
  jobsystem::SharedJobManager m_job_manager;
  props::SharedPropertyProvider m_property_provider;
  messaging::SharedBroker m_message_broker;
  graphics::SharedRenderer m_renderer;
  services::SharedServiceRegistry m_service_registry;
  networking::SharedNetworkingManager m_networking_manager;
  resourcemgmt::SharedResourceManager m_resource_manager;

public:
  PluginContext(jobsystem::SharedJobManager mJobManager,
                props::SharedPropertyProvider mPropertyProvider,
                messaging::SharedBroker mMessageBroker,
                graphics::SharedRenderer mRenderer,
                services::SharedServiceRegistry mServiceRegistry,
                networking::SharedNetworkingManager mNetworkingManager,
                resourcemgmt::SharedResourceManager mResourceManager)
      : m_job_manager(std::move(mJobManager)),
        m_property_provider(std::move(mPropertyProvider)),
        m_message_broker(std::move(mMessageBroker)),
        m_renderer(std::move(mRenderer)),
        m_service_registry(std::move(mServiceRegistry)),
        m_networking_manager(std::move(mNetworkingManager)),
        m_resource_manager(std::move(mResourceManager)) {}

  jobsystem::SharedJobManager GetJobManager() const { return m_job_manager; }

  props::SharedPropertyProvider GetPropertyProvider() const {
    return m_property_provider;
  }

  messaging::SharedBroker GetMessageBroker() const { return m_message_broker; }

  graphics::SharedRenderer GetRenderer() const { return m_renderer; }

  services::SharedServiceRegistry GetServiceRegistry() const {
    return m_service_registry;
  }

  networking::SharedNetworkingManager GetNetworkingManager() const {
    return m_networking_manager;
  }

  resourcemgmt::SharedResourceManager GetResourceManager() const {
    return m_resource_manager;
  }
};

typedef std::shared_ptr<PluginContext> SharedPluginContext;

} // namespace plugins

#endif // PLUGINCONTEXT_H
