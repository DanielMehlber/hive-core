#include "events/Event.h"

#ifndef SERVICEREGISTEREDMESSAGE_H
#define SERVICEREGISTEREDMESSAGE_H

namespace services {
class ServiceRegisteredNotification {
private:
  events::SharedEvent m_message;

public:
  explicit ServiceRegisteredNotification(
      events::SharedEvent message =
          std::make_shared<events::Event>("service-registered-notification"))
      : m_message(std::move(message)) {}

  events::SharedEvent GetMessage();

  void SetServiceName(const std::string &name);
  std::string GetServiceName();
};

inline events::SharedEvent ServiceRegisteredNotification::GetMessage() {
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
