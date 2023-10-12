#ifndef WEBSOCKETMESSAGECONVERTER_H
#define WEBSOCKETMESSAGECONVERTER_H

#include "WebSocketMessage.h"
#include "common/exceptions/ExceptionsBase.h"

namespace networking::websockets {

DECLARE_EXCEPTION(MessagePayloadInvalidException);

/**
 * Converts the payload of web-socket messages to data objects and vice versa.
 */
class WebSocketMessageConverter {
public:
  /**
   * Generates a web-socket data object from json payload
   * @param json payload
   * @return a web-socket message object
   * @throws MessagePayloadInvalidException if the payload is not a valid json
   * or is not convertible to a message object
   */
  static SharedWebSocketMessage FromJson(const std::string &json);

  /**
   * Generates the json string from a web-socket message
   * @param message message object that will be converted into json
   * @return the json string
   */
  static std::string ToJson(SharedWebSocketMessage message);
};
} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGECONVERTER_H */
