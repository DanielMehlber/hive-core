#ifndef IWEBSOCKETMESSAGECONSUMER_H
#define IWEBSOCKETMESSAGECONSUMER_H

#include "PeerConnectionInfo.h"
#include "PeerMessage.h"
#include <memory>

namespace networking::websockets {

/**
 * @brief Generic interface for receiving events that are sent over the
 * network
 */
class IPeerMessageConsumer
    : public std::enable_shared_from_this<IPeerMessageConsumer> {
public:
  /**
   * @brief Returns the web-socket message type this consumer listens to
   * @return string that contains the unique type name
   */
  virtual std::string GetMessageType() const noexcept = 0;

  /**
   * @brief Reacts to received events and processes them
   * @param received_message message
   */
  virtual void
  ProcessReceivedMessage(SharedWebSocketMessage received_message,
                         PeerConnectionInfo connection_info) noexcept = 0;
};

typedef std::shared_ptr<IPeerMessageConsumer> SharedPeerMessageConsumer;

} // namespace networking::websockets

#endif /* IWEBSOCKETMESSAGECONSUMER_H */
