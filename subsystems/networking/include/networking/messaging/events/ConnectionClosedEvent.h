#ifndef SIMULATION_FRAMEWORK_CONNECTIONCLOSEDEVENT_H
#define SIMULATION_FRAMEWORK_CONNECTIONCLOSEDEVENT_H

#include "events/Event.h"

namespace networking {
class ConnectionClosedEvent {
private:
  events::SharedEvent m_event;

public:
  explicit ConnectionClosedEvent(
      events::SharedEvent event = std::make_shared<events::Event>(c_event_name))
      : m_event(std::move(event)) {}

  events::SharedEvent GetEvent() const;

  std::string GetPeerId() const;
  void SetPeerId(const std::string &peer_id);

  static constexpr const char *c_event_name = "connection-closed";
};

inline events::SharedEvent ConnectionClosedEvent::GetEvent() const {
  return m_event;
}

inline std::string ConnectionClosedEvent::GetPeerId() const {
  return m_event->GetPayload<std::string>("peer-id").value();
}

inline void ConnectionClosedEvent::SetPeerId(const std::string &peer_id) {
  m_event->SetPayload<std::string>("peer-id", peer_id);
}

} // namespace networking

#endif // SIMULATION_FRAMEWORK_CONNECTIONCLOSEDEVENT_H
