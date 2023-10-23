#ifndef IWEBSOCKETMESSAGECONSUMER_H
#define IWEBSOCKETMESSAGECONSUMER_H

#include "WebSocketConnectionInfo.h"
#include "WebSocketMessage.h"
#include <memory>

namespace networking::websockets {

/**
 * @brief Generic interface for receiving messages that are sent over the
 * network
 */
class IWebSocketMessageConsumer
    : public std::enable_shared_from_this<IWebSocketMessageConsumer> {
public:
  /**
   * @brief Returns the web-socket message type this consumer listens to
   * @return string that contains the unique type name
   */
  virtual std::string GetMessageType() const noexcept = 0;

  /**
   * @brief Reacts to received messages and processes them
   * @param received_message message
   */
  virtual void
  ProcessReceivedMessage(SharedWebSocketMessage received_message,
                         WebSocketConnectionInfo connection_info) noexcept = 0;
};

typedef std::shared_ptr<IWebSocketMessageConsumer>
    SharedWebSocketMessageConsumer;

} // namespace networking::websockets

#endif /* IWEBSOCKETMESSAGECONSUMER_H */
