#pragma once

#include "Message.h"
#include "common/exceptions/ExceptionsBase.h"

namespace hive::networking::messaging {

DECLARE_EXCEPTION(MessagePayloadInvalidException);

/**
 * Converts the payload of web-socket events to data objects and vice versa.
 */
class MessageConverter {
public:
  /**
   * Generates a web-socket data object from json payload
   * @param json payload
   * @return a web-socket message object
   * @throws MessagePayloadInvalidException if the payload is not a valid json
   * or is not convertible to a message object
   * @deprecated Is not capable to transport binary data without encoding
   */
  static SharedMessage FromJson(const std::string &json);

  /**
   * Generates the json string from a web-socket message
   * @param message message object that will be converted into json
   * @return the json string
   * @deprecated Is not capable to transport binary data without encoding
   */
  static std::string ToJson(const SharedMessage &message);

  /**
   * Generates a message data object from multipart-formdata content
   * @param multipart_string payload
   * @return a message data object
   */
  static SharedMessage
  FromMultipartFormData(const std::string &multipart_string);

  /**
   * Generates the multipart-formdata encoded string from a message
   * @param message message object that will be converted into json
   * @return the multipart-formdata encoded string
   */
  static std::string ToMultipartFormData(const SharedMessage &message);
};
} // namespace hive::networking::messaging
