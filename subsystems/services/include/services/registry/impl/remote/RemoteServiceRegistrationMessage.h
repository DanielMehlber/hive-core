#pragma once

#include "networking/messaging/Message.h"

#define MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION "register-remote-service"

namespace hive::services::impl {

/**
 * Web-Socket message sent, when a node wants to offer some if its services
 * to another one.
 */
class RemoteServiceRegistrationMessage {
private:
  networking::messaging::SharedMessage m_message;

public:
  explicit RemoteServiceRegistrationMessage(
      networking::messaging::SharedMessage message =
          std::make_shared<networking::messaging::Message>(
              MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION));

  networking::messaging::SharedMessage GetMessage() const;

  void SetId(const std::string &id);
  std::string GetId();

  void SetServiceName(const std::string &service_name);
  std::string GetServiceName();

  void SetCapacity(size_t capacity);
  size_t GetCapacity();
};

inline void RemoteServiceRegistrationMessage::SetServiceName(
    const std::string &service_name) {
  m_message->SetAttribute("service-name", service_name);
}

inline std::string RemoteServiceRegistrationMessage::GetServiceName() {
  return m_message->GetAttribute("service-name").value_or("");
}

inline RemoteServiceRegistrationMessage::RemoteServiceRegistrationMessage(
    networking::messaging::SharedMessage message)
    : m_message(std::move(message)) {}

inline networking::messaging::SharedMessage
RemoteServiceRegistrationMessage::GetMessage() const {
  return m_message;
}

inline void RemoteServiceRegistrationMessage::SetId(const std::string &id) {
  m_message->SetAttribute("service-id", id);
}

inline std::string RemoteServiceRegistrationMessage::GetId() {
  return m_message->GetAttribute("service-id").value_or("");
}

inline void RemoteServiceRegistrationMessage::SetCapacity(size_t capacity) {
  m_message->SetAttribute("service-capacity", std::to_string(capacity));
}

inline size_t RemoteServiceRegistrationMessage::GetCapacity() {
  return std::stoul(m_message->GetAttribute("service-capacity").value_or("-1"));
}

} // namespace hive::services::impl
