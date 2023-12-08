#ifndef SIMULATION_FRAMEWORK_CONNECTIONESTABLISHEDEVENT_H
#define SIMULATION_FRAMEWORK_CONNECTIONESTABLISHEDEVENT_H

#include "events/Event.h"

namespace networking {
class ConnectionEstablishedEvent {
private:
  events::SharedEvent m_message;

public:
  explicit ConnectionEstablishedEvent(
      events::SharedEvent message =
          std::make_shared<events::Event>("connection-established"))
      : m_message(std::move(message)) {}

  events::SharedEvent GetEvent() const;

  std::string GetPeerId() const;
  void SetPeer(const std::string &peer_id);
};

inline events::SharedEvent ConnectionEstablishedEvent::GetEvent() const {
  return m_message;
}

inline std::string ConnectionEstablishedEvent::GetPeerId() const {
  return m_message->GetPayload<std::string>("peer-id").value();
}

inline void ConnectionEstablishedEvent::SetPeer(const std::string &peer_id) {
  m_message->SetPayload<std::string>("peer-id", peer_id);
}

} // namespace networking

#endif // SIMULATION_FRAMEWORK_CONNECTIONESTABLISHEDEVENT_H
