#ifndef WEBSOCKETSERVICERESPONSECONSUMER_H
#define WEBSOCKETSERVICERESPONSECONSUMER_H

#include "networking/websockets/IWebSocketMessageConsumer.h"
#include "services/ServiceResponse.h"
#include <future>

#define MESSAGE_TYPE_WS_SERVICE_RESPONSE "service-response"

using namespace networking::websockets;

namespace services::impl {
class WebSocketServiceResponseConsumer : public IWebSocketMessageConsumer {
private:
  std::map<std::string, std::promise<SharedServiceResponse>> m_promises;

public:
  std::string GetMessageType() const noexcept override;

  void ProcessReceivedMessage(
      SharedWebSocketMessage received_message,
      WebSocketConnectionInfo connection_info) noexcept override;

  /**
   * Adds response promise to this consumer. The consumer will resolve this
   * promise when its response has been received and consumed.
   * @param request_id id of request (response will have same id)
   * @param response_promise response promise
   */
  void
  AddResponsePromise(const std::string &request_id,
                     std::promise<SharedServiceResponse> &&response_promise);
};

inline std::string
WebSocketServiceResponseConsumer::GetMessageType() const noexcept {
  return MESSAGE_TYPE_WS_SERVICE_RESPONSE;
}

} // namespace services::impl

#endif // WEBSOCKETSERVICERESPONSECONSUMER_H
