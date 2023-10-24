#ifndef WEBSOCKETSERVICESTUB_H
#define WEBSOCKETSERVICESTUB_H

#include "networking/websockets/IWebSocketPeer.h"
#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"
#include "services/stub/IServiceStub.h"
#include <memory>

using namespace networking::websockets;

namespace services::impl {

DECLARE_EXCEPTION(CallFailedException);

/**
 * Service Stub for services that can be called using websockets.
 */
class WebSocketServiceStub : public IServiceStub {
private:
  /** Peer that will be used to send calls to remote services */
  std::weak_ptr<IWebSocketPeer> m_web_socket_peer;

  /** Hostname of remote endpoint */
  std::string m_remote_host_name;

  /** name of called service */
  std::string m_service_name;

  /**
   * Stub will pass promise (for resolving the request's response) to the
   * response consumer. It receives responses and can therefore resolve the
   * promise.
   */
  std::weak_ptr<WebSocketServiceResponseConsumer> m_response_consumer;

public:
  WebSocketServiceStub() = delete;
  explicit WebSocketServiceStub(
      std::string service_name, std::weak_ptr<IWebSocketPeer> peer,
      std::string remote_host_name,
      std::weak_ptr<WebSocketServiceResponseConsumer> response_consumer);

  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) override;

  bool IsUsable() override;

  std::string GetServiceName() override;

  bool IsLocal() override { return false; };
};
} // namespace services::impl

#endif // WEBSOCKETSERVICESTUB_H
