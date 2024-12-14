#pragma once

#include "events/Event.h"

namespace hive::networking {
class ConnectionEstablishedEvent {
  events::SharedEvent m_event;

public:
  explicit ConnectionEstablishedEvent(
      events::SharedEvent message =
          std::make_shared<events::Event>("connection-established"))
      : m_event(std::move(message)) {}

  events::SharedEvent GetEvent() const;

  std::string GetEndpointId() const;
  void SetEndpointId(const std::string &endpoint_id);
};

inline events::SharedEvent ConnectionEstablishedEvent::GetEvent() const {
  return m_event;
}

inline std::string ConnectionEstablishedEvent::GetEndpointId() const {
  return m_event->GetPayload<std::string>("endpoint-id").value();
}

inline void
ConnectionEstablishedEvent::SetEndpointId(const std::string &endpoint_id) {
  m_event->SetPayload<std::string>("endpoint-id", endpoint_id);
}

} // namespace hive::networking
