#ifndef WEBSOCKETSERVICEEXECUTOR_H
#define WEBSOCKETSERVICEEXECUTOR_H

#include "networking/websockets/IWebSocketPeer.h"
#include "services/Services.h"
#include "services/executor/IServiceExecutor.h"
#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"
#include <memory>

using namespace networking::websockets;

namespace services::impl {

DECLARE_EXCEPTION(CallFailedException);

/**
 * Executes remote services using web-socket messages.
 */
class SERVICES_API WebSocketServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<WebSocketServiceExecutor> {
private:
  /** Peer that will be used to send calls to remote services */
  std::weak_ptr<IWebSocketPeer> m_web_socket_peer;

  /** Hostname of remote endpoint (recipient of call message) */
  std::string m_remote_host_name;

  /** name of called service */
  std::string m_service_name;

  /**
   * This execution will pass a promise (for resolving the request's response)
   * to the response consumer. It receives responses and can therefore resolve
   * the promise.
   */
  std::weak_ptr<WebSocketServiceResponseConsumer> m_response_consumer;

public:
  WebSocketServiceExecutor() = delete;
  explicit WebSocketServiceExecutor(
      std::string service_name, std::weak_ptr<IWebSocketPeer> peer,
      std::string remote_host_name,
      std::weak_ptr<WebSocketServiceResponseConsumer> response_consumer);

  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) override;

  bool IsCallable() override;

  std::string GetServiceName() override;

  bool IsLocal() override { return false; };
};
} // namespace services::impl

#endif // WEBSOCKETSERVICEEXECUTOR_H
