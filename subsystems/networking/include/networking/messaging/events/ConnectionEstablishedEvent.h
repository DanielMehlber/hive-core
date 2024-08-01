#pragma once

#include "events/Event.h"

namespace networking {
class ConnectionEstablishedEvent {
private:
  events::SharedEvent m_event;

public:
  explicit ConnectionEstablishedEvent(
      events::SharedEvent message =
          std::make_shared<events::Event>("connection-established"))
      : m_event(std::move(message)) {}

  events::SharedEvent GetEvent() const;

  std::string GetEndpointId() const;
  void SetEndpointId(const std::string &peer_id);
};

inline events::SharedEvent ConnectionEstablishedEvent::GetEvent() const {
  return m_event;
}

inline std::string ConnectionEstablishedEvent::GetEndpointId() const {
  return m_event->GetPayload<std::string>("peer-id").value();
}

inline void
ConnectionEstablishedEvent::SetEndpointId(const std::string &peer_id) {
  m_event->SetPayload<std::string>("peer-id", peer_id);
}

} // namespace networking
