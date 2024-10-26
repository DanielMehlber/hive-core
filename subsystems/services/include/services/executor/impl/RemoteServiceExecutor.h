#pragma once

#include "networking/NetworkingManager.h"
#include "services/executor/IServiceExecutor.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"
#include <memory>

namespace hive::services::impl {

/**
 * Executes remote services using web-socket events.
 */
class RemoteServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<RemoteServiceExecutor> {
  /** Endpoint that will be used to send calls to remote services */
  common::memory::Reference<networking::NetworkingManager> m_networking_manager;

  /** Hostname of remote endpoint (recipient of call message) */
  networking::messaging::ConnectionInfo m_remote_host_info;

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

  /**
   * Amount of concurrent calls that the remote executor can process in
   * parallel. Some services are more heavy-weight than others and may need to
   * limit the number of concurrent calls.
   */
  capacity_t m_capacity;

public:
  RemoteServiceExecutor() = delete;
  explicit RemoteServiceExecutor(
      std::string service_name,
      common::memory::Reference<networking::NetworkingManager> networking_mgr,
      networking::messaging::ConnectionInfo remote_host_info, std::string id,
      std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
      size_t capacity = -1);

  ~RemoteServiceExecutor() override = default;

  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool async) override;

  bool IsCallable() override;
  std::string GetServiceName() override;
  bool IsLocal() override;
  std::string GetId() override;
  capacity_t GetCapacity() const override;
};

inline std::string RemoteServiceExecutor::GetId() { return m_id; }
inline std::string RemoteServiceExecutor::GetServiceName() {
  return m_service_name;
}
inline bool RemoteServiceExecutor::IsLocal() { return false; }
inline capacity_t RemoteServiceExecutor::GetCapacity() const {
  return m_capacity;
}

} // namespace hive::services::impl
