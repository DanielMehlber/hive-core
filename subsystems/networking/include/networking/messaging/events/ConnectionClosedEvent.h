#pragma once

#include "events/Event.h"

namespace hive::networking {
class ConnectionClosedEvent {
  events::SharedEvent m_event;

public:
  explicit ConnectionClosedEvent(
      events::SharedEvent event = std::make_shared<events::Event>(c_event_name))
      : m_event(std::move(event)) {}

  events::SharedEvent GetEvent() const;

  std::string GetEndpointId() const;
  void SetEndpointId(const std::string &endpoint_id);

  static constexpr const char *c_event_name = "connection-closed";
};

inline events::SharedEvent ConnectionClosedEvent::GetEvent() const {
  return m_event;
}

inline std::string ConnectionClosedEvent::GetEndpointId() const {
  return m_event->GetPayload<std::string>("endpoint-id").value();
}

inline void
ConnectionClosedEvent::SetEndpointId(const std::string &endpoint_id) {
  m_event->SetPayload<std::string>("endpoint-id", endpoint_id);
}

} // namespace hive::networking
