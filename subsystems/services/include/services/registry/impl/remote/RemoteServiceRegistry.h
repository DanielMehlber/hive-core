#ifndef WEBSOCKETSERVICEREGISTRY_H
#define WEBSOCKETSERVICEREGISTRY_H

#include "common/memory/ExclusiveOwnership.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/remote/RemoteServiceRegistrationConsumer.h"
#include "services/registry/impl/remote/RemoteServiceRequestConsumer.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"

namespace services::impl {

/**
 * A registry for both local and remote web-socket services.
 */
class RemoteServiceRegistry : public IServiceRegistry {
protected:
  mutable jobsystem::mutex m_registered_callers_mutex;
  std::map<std::string, SharedServiceCaller> m_registered_callers;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  SharedMessageConsumer m_registration_consumer;
  SharedMessageConsumer m_response_consumer;
  SharedMessageConsumer m_request_consumer;

  events::SharedEventListener m_new_peer_connection_listener;

  static SharedJob CreateRemoteServiceRegistrationJob(
      const std::string &peer_id, const std::string &service_name,
      common::memory::Reference<networking::messaging::IMessageEndpoint>
          sending_peer);

public:
  explicit RemoteServiceRegistry(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems);

  void Register(const SharedServiceExecutor &stub) override;
  void Unregister(const std::string &name) override;

  void SendServicePortfolioToPeer(const std::string &id);

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
  void SetupMessageConsumers();
  void SetupEventSubscribers();
};

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRY_H
