#ifndef WEBSOCKETSERVICEREGISTRATIONCONSUMER_H
#define WEBSOCKETSERVICEREGISTRATIONCONSUMER_H

#include "networking/peers/IMessageConsumer.h"
#include "networking/peers/IMessageEndpoint.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/remote/RemoteServiceRegistrationMessage.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"

using namespace networking::websockets;

namespace services::impl {

/**
 * Consumer for web-socket events that register remote services on the current
 * host.
 */
class RemoteServiceRegistrationConsumer : public IMessageConsumer {
private:
  std::function<void(SharedServiceExecutor)> m_consumer;
  std::weak_ptr<RemoteServiceResponseConsumer> m_response_consumer;
  std::weak_ptr<IMessageEndpoint> m_web_socket_peer;

public:
  RemoteServiceRegistrationConsumer() = delete;
  explicit RemoteServiceRegistrationConsumer(
      std::function<void(SharedServiceExecutor)> consumer,
      std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
      std::weak_ptr<IMessageEndpoint> web_socket_peer);

  std::string GetMessageType() const noexcept override;

  void ProcessReceivedMessage(SharedMessage received_message,
                              ConnectionInfo connection_info) noexcept override;
};

inline std::string
RemoteServiceRegistrationConsumer::GetMessageType() const noexcept {
  return MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION;
}
} // namespace services::impl
#endif // WEBSOCKETSERVICEREGISTRATIONCONSUMER_H
