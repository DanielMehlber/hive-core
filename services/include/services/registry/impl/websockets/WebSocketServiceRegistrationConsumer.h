#ifndef WEBSOCKETSERVICEREGISTRATIONCONSUMER_H
#define WEBSOCKETSERVICEREGISTRATIONCONSUMER_H

#include "networking/websockets/IWebSocketMessageConsumer.h"
#include "networking/websockets/IWebSocketPeer.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistrationMessage.h"
#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"

using namespace networking::websockets;

namespace services::impl {

/**
 * Consumer for web-socket messages that register remote services also
 * accessible using web-socket messages.
 */
class WebSocketServiceRegistrationConsumer : public IWebSocketMessageConsumer {
private:
  std::function<void(SharedServiceStub)> m_consumer;
  std::weak_ptr<WebSocketServiceResponseConsumer> m_response_consumer;
  std::weak_ptr<IWebSocketPeer> m_web_socket_peer;

public:
  WebSocketServiceRegistrationConsumer() = delete;
  explicit WebSocketServiceRegistrationConsumer(
      std::function<void(SharedServiceStub)> consumer,
      std::weak_ptr<WebSocketServiceResponseConsumer> response_consumer,
      std::weak_ptr<IWebSocketPeer> web_socket_peer);

  std::string GetMessageType() const noexcept override;

  void ProcessReceivedMessage(
      SharedWebSocketMessage received_message,
      WebSocketConnectionInfo connection_info) noexcept override;
};

inline std::string
WebSocketServiceRegistrationConsumer::GetMessageType() const noexcept {
  return MESSAGE_TYPE_WS_SERVICE_REGISTRATION;
}
} // namespace services::impl
#endif // WEBSOCKETSERVICEREGISTRATIONCONSUMER_H
