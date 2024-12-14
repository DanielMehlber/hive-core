#pragma once
#include "common/exceptions/ExceptionsBase.h"
#include "networking/messaging/Message.h"

namespace hive::networking::messaging {

DECLARE_EXCEPTION(MessagePayloadInvalidException);

/**
 * Generic interface for serializing and deserializing messages received via
 * endpoints.
 */
class IMessageConverter {
public:
  /**
   * @return content type of the message that this converter can handle
   */
  virtual std::string GetContentType() = 0;

  /**
   * Parses a string representation of a message into a message object.
   * @param data string representation of the message in some implementation
   * specific format.
   * @return message object
   */
  virtual SharedMessage Deserialize(const std::string &data) = 0;

  /**
   * Serializes a message object into a string representation.
   * @param message message object to serialize
   * @return string representation of the message
   */
  virtual std::string Serialize(const SharedMessage &message) = 0;

  virtual ~IMessageConverter() = default;
};

} // namespace hive::networking::messaging