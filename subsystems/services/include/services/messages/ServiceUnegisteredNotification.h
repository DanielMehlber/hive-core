#include "messaging/Message.h"

#ifndef SERVICEUNREGISTEREDMESSAGE_H
#define SERVICEUNREGISTEREDMESSAGE_H

namespace services {
class ServiceUnregisteredNotification {
private:
  messaging::SharedMessage m_message;

public:
  explicit ServiceUnregisteredNotification(
      messaging::SharedMessage message = std::make_shared<messaging::Message>(
          "service-unregistered-notification"))
      : m_message(std::move(message)) {}

  messaging::SharedMessage GetMessage();

  void SetServiceName(const std::string &name);
  std::string GetServiceName();
};

inline messaging::SharedMessage ServiceUnregisteredNotification::GetMessage() {
  return m_message;
}

inline void
ServiceUnregisteredNotification::SetServiceName(const std::string &name) {
  m_message->SetPayload("service-name", name);
}

inline std::string ServiceUnregisteredNotification::GetServiceName() {
  return m_message->GetPayload<std::string>("service-name").value_or("");
}
} // namespace services

#endif // SERVICEUNREGISTEREDMESSAGE_H
