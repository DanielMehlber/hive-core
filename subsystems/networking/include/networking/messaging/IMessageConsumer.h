#pragma once

#include "ConnectionInfo.h"
#include "Message.h"
#include <memory>

namespace hive::networking::messaging {

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
  virtual std::string GetMessageType() const = 0;

  /**
   * Processes a message that was received by the message endpoint.
   * @param received_message message that must be processed
   * @param connection_info information about the sender of the message
   */
  virtual void ProcessReceivedMessage(SharedMessage received_message,
                                      ConnectionInfo connection_info) = 0;

  virtual ~IMessageConsumer() = default;
};

typedef std::shared_ptr<IMessageConsumer> SharedMessageConsumer;

} // namespace hive::networking::messaging
