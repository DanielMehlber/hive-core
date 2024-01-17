#ifndef WEBSOCKETSERVICEREGISTRY_H
#define WEBSOCKETSERVICEREGISTRY_H

#include "events/listener/impl/FunctionalEventListener.h"
#include "networking/peers/IPeer.h"
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
  mutable std::mutex m_registered_callers_mutex;
  std::map<std::string, SharedServiceCaller> m_registered_callers;

  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  SharedPeerMessageConsumer m_registration_consumer;
  SharedPeerMessageConsumer m_response_consumer;
  SharedPeerMessageConsumer m_request_consumer;

  events::SharedEventListener m_new_peer_connection_listener;

  static SharedJob CreateRemoteServiceRegistrationJob(
      const std::string &peer_id, const std::string &service_name,
      const std::weak_ptr<networking::websockets::IPeer> &sending_peer);

public:
  explicit RemoteServiceRegistry(
      const common::subsystems::SharedSubsystemManager &subsystems);

  void Register(const SharedServiceExecutor &stub) override;
  void Unregister(const std::string &name) override;

  void SendServicePortfolioToPeer(const std::string &id) const;

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
  void SetupMessageConsumers();
  void SetupEventSubscribers();
};

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRY_H
