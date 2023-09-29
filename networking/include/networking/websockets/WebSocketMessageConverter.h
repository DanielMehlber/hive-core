#ifndef WEBSOCKETMESSAGECONVERTER_H
#define WEBSOCKETMESSAGECONVERTER_H

#include "WebSocketMessage.h"
#include "common/exceptions/ExceptionsBase.h"

namespace networking::websockets {

DECLARE_EXCEPTION(MessagePayloadInvalidException);

class WebSocketMessageConverter {
public:
  static SharedWebSocketMessage FromJson(const std::string &json);
  static std::string ToJson(SharedWebSocketMessage message);
};
} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGECONVERTER_H */
