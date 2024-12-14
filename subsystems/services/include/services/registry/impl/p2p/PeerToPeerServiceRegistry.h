#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "networking/messaging/endpoints/IMessageEndpoint.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/remote/RemoteServiceRegistrationConsumer.h"
#include "services/registry/impl/remote/RemoteServiceRequestConsumer.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"

namespace hive::services::impl {

/**
 * This service registry informs its directly connected peers about new
 * services using messaging. It also listens for new services from its peers.
 * There is no central component of third-party registry involved.
 *
 * @note This is a first and simple implementation of the service registry.
 * There may be more elegant solutions using specialized protocols or third
 * party registries.
 */
class PeerToPeerServiceRegistry : public IServiceRegistry {
protected:
  mutable jobsystem::mutex m_registered_callers_mutex;
  std::map<std::string, SharedServiceCaller> m_registered_callers;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  networking::messaging::SharedMessageConsumer m_registration_consumer;
  networking::messaging::SharedMessageConsumer m_response_consumer;
  networking::messaging::SharedMessageConsumer m_request_consumer;

  events::SharedEventListener m_new_endpoint_connection_listener;

  static jobsystem::SharedJob CreateRemoteServiceRegistrationJob(
      const std::string &endpoint_id, const std::string &service_name,
      const std::string &service_id, size_t capactiy,
      common::memory::Reference<networking::messaging::IMessageEndpoint>
          sending_endpoint);

public:
  explicit PeerToPeerServiceRegistry(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems);

  void Register(const SharedServiceExecutor &executor) override;
  void UnregisterAll(const std::string &name) override;
  void Unregister(const std::string &executor_id) override;
  void Unregister(const SharedServiceExecutor &executor) override;

  void SendServicePortfolioToEndpoint(const std::string &endpoint_id);

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &service_name, bool only_local) override;
  void SetupMessageConsumers();
  void SetupEventSubscribers();
};

} // namespace hive::services::impl
