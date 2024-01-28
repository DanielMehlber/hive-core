#ifndef IWEBSOCKETMESSAGECONSUMER_H
#define IWEBSOCKETMESSAGECONSUMER_H

#include "ConnectionInfo.h"
#include "Message.h"
#include <memory>

namespace networking::websockets {

/**
 * Generic interface for receiving events that are sent over the
 * network
 */
class IMessageConsumer : public std::enable_shared_from_this<IMessageConsumer> {
public:
  /**
   * Returns the web-socket message type this consumer listens to
   * @return string that contains the unique type name
   */
  virtual std::string GetMessageType() const noexcept = 0;

  /**
   * Reacts to received events and processes them
   * @param received_message message
   */
  virtual void
  ProcessReceivedMessage(SharedMessage received_message,
                         ConnectionInfo connection_info) noexcept = 0;
};

typedef std::shared_ptr<IMessageConsumer> SharedMessageConsumer;

} // namespace networking::websockets

#endif /* IWEBSOCKETMESSAGECONSUMER_H */
