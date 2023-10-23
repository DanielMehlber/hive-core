#ifndef WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
#define WEBSOCKETSERVICEREGISTRATIONMESSAGE_H

#include <utility>

#include "networking/websockets/WebSocketMessage.h"

using namespace networking::websockets;

#define MESSAGE_TYPE_WS_SERVICE_REGISTRATION "register-web-socket-service"

namespace services::impl {

/**
 * Web-Socket message sent, when a peer wants to offer some if its services to
 * another peer.
 */
class WebSocketServiceRegistrationMessage {
private:
  SharedWebSocketMessage m_message;

public:
  explicit WebSocketServiceRegistrationMessage(SharedWebSocketMessage message);

  SharedWebSocketMessage GetMessage() const noexcept;

  void SetServiceName(const std::string &service_name) noexcept;
  std::string GetServiceName() noexcept;
};

inline void WebSocketServiceRegistrationMessage::SetServiceName(
    const std::string &service_name) noexcept {
  m_message->SetAttribute("service-name", service_name);
}

inline std::string
WebSocketServiceRegistrationMessage::GetServiceName() noexcept {
  return m_message->GetAttribute("service-name").value_or("");
}

inline WebSocketServiceRegistrationMessage::WebSocketServiceRegistrationMessage(
    SharedWebSocketMessage message)
    : m_message(std::move(message)) {}

inline SharedWebSocketMessage
WebSocketServiceRegistrationMessage::GetMessage() const noexcept {
  return m_message;
}

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
