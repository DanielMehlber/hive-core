#pragma once

#include "networking/messaging/Message.h"

using namespace hive::networking::messaging;

#define MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION "register-remote-service"

namespace hive::services::impl {

/**
 * Web-Socket message sent, when a node wants to offer some if its services
 * to another one.
 */
class RemoteServiceRegistrationMessage {
private:
  SharedMessage m_message;

public:
  explicit RemoteServiceRegistrationMessage(
      SharedMessage message =
          std::make_shared<Message>(MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION));

  SharedMessage GetMessage() const;

  void SetId(const std::string &id);
  std::string GetId();

  void SetServiceName(const std::string &service_name);
  std::string GetServiceName();
};

inline void RemoteServiceRegistrationMessage::SetServiceName(
    const std::string &service_name) {
  m_message->SetAttribute("service-name", service_name);
}

inline std::string RemoteServiceRegistrationMessage::GetServiceName() {
  return m_message->GetAttribute("service-name").value_or("");
}

inline RemoteServiceRegistrationMessage::RemoteServiceRegistrationMessage(
    SharedMessage message)
    : m_message(std::move(message)) {}

inline SharedMessage RemoteServiceRegistrationMessage::GetMessage() const {
  return m_message;
}

inline void RemoteServiceRegistrationMessage::SetId(const std::string &id) {
  m_message->SetAttribute("service-id", id);
}

inline std::string RemoteServiceRegistrationMessage::GetId() {
  return m_message->GetAttribute("service-id").value_or("");
}

} // namespace hive::services::impl
