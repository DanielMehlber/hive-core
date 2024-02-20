#ifndef WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
#define WEBSOCKETSERVICEREGISTRATIONMESSAGE_H

#include "networking/messaging/Message.h"

using namespace networking::websockets;

#define MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION "register-remote-service"

namespace services::impl {

/**
 * Web-Socket message sent, when a peer wants to offer some if its services to
 * another one.
 */
class RemoteServiceRegistrationMessage {
private:
  SharedMessage m_message;

public:
  explicit RemoteServiceRegistrationMessage(
      SharedMessage message =
          std::make_shared<Message>(MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION));

  SharedMessage GetMessage() const noexcept;

  void SetId(const std::string &id) noexcept;
  std::string GetId() noexcept;

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
    SharedMessage message)
    : m_message(std::move(message)) {}

inline SharedMessage
RemoteServiceRegistrationMessage::GetMessage() const noexcept {
  return m_message;
}

inline void
RemoteServiceRegistrationMessage::SetId(const std::string &id) noexcept {
  m_message->SetAttribute("service-id", id);
}

inline std::string RemoteServiceRegistrationMessage::GetId() noexcept {
  return m_message->GetAttribute("service-id").value_or("");
}

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRATIONMESSAGE_H
