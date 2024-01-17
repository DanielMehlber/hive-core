#ifndef WEBSOCKETSERVICERESPONSECONSUMER_H
#define WEBSOCKETSERVICERESPONSECONSUMER_H

#include "networking/peers/IPeerMessageConsumer.h"
#include "services/ServiceResponse.h"
#include <future>

using namespace networking::websockets;

namespace services::impl {

/**
 * Processes incoming service responses of recently called remote services.
 */
class RemoteServiceResponseConsumer : public IPeerMessageConsumer {
private:
  mutable std::mutex m_promises_mutex;
  std::map<std::string, std::promise<SharedServiceResponse>> m_promises;

public:
  std::string GetMessageType() const noexcept override;

  void
  ProcessReceivedMessage(SharedWebSocketMessage received_message,
                         PeerConnectionInfo connection_info) noexcept override;

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
RemoteServiceResponseConsumer::GetMessageType() const noexcept {
  return "service-response";
}

} // namespace services::impl

#endif // WEBSOCKETSERVICERESPONSECONSUMER_H
