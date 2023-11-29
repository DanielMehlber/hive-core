#ifndef WEBSOCKETSERVICEREGISTRY_H
#define WEBSOCKETSERVICEREGISTRY_H

#include "networking/peers/IPeer.h"
#include "services/Services.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistrationConsumer.h"
#include "services/registry/impl/websockets/WebSocketServiceRequestConsumer.h"
#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"

namespace services::impl {

/**
 * A registry for both local and remote web-socket services.
 */
class SERVICES_API WebSocketServiceRegistry : public IServiceRegistry {
protected:
  mutable std::mutex m_registered_callers_mutex;
  std::map<std::string, SharedServiceCaller> m_registered_callers;

  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  SharedWebSocketMessageConsumer m_registration_consumer;
  SharedWebSocketMessageConsumer m_response_consumer;
  SharedWebSocketMessageConsumer m_request_consumer;

public:
  explicit WebSocketServiceRegistry(
      const common::subsystems::SharedSubsystemManager &subsystems);

  void Register(const SharedServiceExecutor &stub) override;

  void Unregister(const std::string &name) override;

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
};

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRY_H
