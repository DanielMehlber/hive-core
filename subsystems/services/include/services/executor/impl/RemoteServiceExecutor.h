#ifndef WEBSOCKETSERVICEEXECUTOR_H
#define WEBSOCKETSERVICEEXECUTOR_H

#include "networking/peers/IPeer.h"
#include "services/Services.h"
#include "services/executor/IServiceExecutor.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"
#include <memory>

using namespace networking::websockets;

namespace services::impl {

DECLARE_EXCEPTION(CallFailedException);

/**
 * Executes remote services using web-socket events.
 */
class SERVICES_API RemoteServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<RemoteServiceExecutor> {
private:
  /** Peer that will be used to send calls to remote services */
  std::weak_ptr<IPeer> m_web_socket_peer;

  /** Hostname of remote endpoint (recipient of call message) */
  std::string m_remote_host_name;

  /** name of called service */
  std::string m_service_name;

  /**
   * This execution will pass a promise (for resolving the request's response)
   * to the response consumer. It receives responses and can therefore resolve
   * the promise.
   */
  std::weak_ptr<RemoteServiceResponseConsumer> m_response_consumer;

public:
  RemoteServiceExecutor() = delete;
  explicit RemoteServiceExecutor(
      std::string service_name, std::weak_ptr<IPeer> peer,
      std::string remote_host_name,
      std::weak_ptr<RemoteServiceResponseConsumer> response_consumer);

  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) override;

  bool IsCallable() override;

  std::string GetServiceName() override;

  bool IsLocal() override { return false; };
};
} // namespace services::impl

#endif // WEBSOCKETSERVICEEXECUTOR_H