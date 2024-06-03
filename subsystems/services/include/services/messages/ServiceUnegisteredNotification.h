#pragma once

#include "events/Event.h"

namespace services {
class ServiceUnregisteredNotification {
private:
  events::SharedEvent m_message;

public:
  explicit ServiceUnregisteredNotification(
      events::SharedEvent message =
          std::make_shared<events::Event>("service-unregistered-notification"))
      : m_message(std::move(message)) {}

  events::SharedEvent GetMessage();

  void SetServiceName(const std::string &name);
  std::string GetServiceName();
};

inline events::SharedEvent ServiceUnregisteredNotification::GetMessage() {
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