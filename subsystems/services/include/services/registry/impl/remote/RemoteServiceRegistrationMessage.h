#ifndef WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
#define WEBSOCKETSERVICEREGISTRATIONMESSAGE_H

#include <utility>

#include "networking/peers/PeerMessage.h"

using namespace networking::websockets;

#define MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION "register-remote-service"

namespace services::impl {

/**
 * Web-Socket message sent, when a peer wants to offer some if its services to
 * another one.
 */
class RemoteServiceRegistrationMessage {
private:
  SharedWebSocketMessage m_message;

public:
  explicit RemoteServiceRegistrationMessage(
      SharedWebSocketMessage message = std::make_shared<PeerMessage>(
          MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION));

  SharedWebSocketMessage GetMessage() const noexcept;

  void SetServiceName(const std::string &service_name) noexcept;
  std::string GetServiceName() noexcept;
};

inline void RemoteServiceRegistrationMessage::SetServiceName(
    const std::string &service_name) noexcept {
  m_message->SetAttribute("service-name", service_name);
}

inline std::string RemoteServiceRegistrationMessage::GetServiceName() noexcept {
  return m_message->GetAttribute("service-name").value_or("");
}

inline RemoteServiceRegistrationMessage::RemoteServiceRegistrationMessage(
    SharedWebSocketMessage message)
    : m_message(std::move(message)) {}

inline SharedWebSocketMessage
RemoteServiceRegistrationMessage::GetMessage() const noexcept {
  return m_message;
}

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
