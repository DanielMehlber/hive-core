#include "messaging/Message.h"

#ifndef SERVICEREGISTEREDMESSAGE_H
#define SERVICEREGISTEREDMESSAGE_H

namespace services {
class ServiceRegisteredNotification {
private:
  messaging::SharedMessage m_message;

public:
  explicit ServiceRegisteredNotification(
      messaging::SharedMessage message = std::make_shared<messaging::Message>(
          "service-registered-notification"))
      : m_message(std::move(message)) {}

  messaging::SharedMessage GetMessage();

  void SetServiceName(const std::string &name);
  std::string GetServiceName();
};

inline messaging::SharedMessage ServiceRegisteredNotification::GetMessage() {
  return m_message;
}

inline void
ServiceRegisteredNotification::SetServiceName(const std::string &name) {
  m_message->SetPayload("service-name", name);
}

inline std::string ServiceRegisteredNotification::GetServiceName() {
  return m_message->GetPayload<std::string>("service-name").value_or("");
}
} // namespace services

#endif // SERVICEREGISTEREDMESSAGE_H
