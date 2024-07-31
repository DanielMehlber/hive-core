#pragma once

#include "networking/messaging/IMessageEndpoint.h"
#include "services/executor/IServiceExecutor.h"
#include "services/registry/impl/remote/RemoteExceptions.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"
#include <memory>

using namespace networking::messaging;

namespace services::impl {

/**
 * Executes remote services using web-socket events.
 */
class RemoteServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<RemoteServiceExecutor> {
private:
  /** Endpoint that will be used to send calls to remote services */
  common::memory::Reference<IMessageEndpoint> m_endpoint;

  /** Hostname of remote endpoint (recipient of call message) */
  ConnectionInfo m_remote_host_info;

  /** name of called service */
  std::string m_service_name;

  /** unique id of this service's executor locally residing on another node */
  std::string m_id;

  /**
   * This execution will pass a promise (for resolving the request's response)
   * to the response consumer. It receives responses and can therefore resolve
   * the promise.
   */
  std::weak_ptr<RemoteServiceResponseConsumer> m_response_consumer;

public:
  RemoteServiceExecutor() = delete;
  explicit RemoteServiceExecutor(
      std::string service_name,
      common::memory::Reference<IMessageEndpoint> endpoint,
      ConnectionInfo remote_host_info, std::string id,
      std::weak_ptr<RemoteServiceResponseConsumer> response_consumer);

  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool async) override;

  bool IsCallable() override;

  std::string GetServiceName() override;

  bool IsLocal() override { return false; };

  std::string GetId() override { return m_id; }
};
} // namespace services::impl
