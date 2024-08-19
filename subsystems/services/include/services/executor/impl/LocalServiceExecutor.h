#pragma once

#include "services/executor/IServiceExecutor.h"
#include <functional>

namespace hive::services::impl {

typedef std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
    service_functor_t;

/**
 * Executes services using a direct function call.
 * @attention service must be located on the same node. This executor does not
 * support remote service calls.
 */
class LocalServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<LocalServiceExecutor> {
protected:
  /** Function that will be called to execute this service */
  service_functor_t m_func;

  /** service name */
  std::string m_service_name;

  /** unique id of this local service executor */
  std::string m_id;

  /** used to limit calls to this service. This is the maximum amount of
   * concurrent calls that are processed by this service. If this limit is
   * reached, the service will answer with a busy response. */
  capacity_t m_capacity;

  /**
   * current active calls to the service. This is used to limit the amount of
   * concurrent calls.
   */
  std::atomic_size_t m_current_concurrent_calls{0};
  mutable jobsystem::mutex m_concurrent_calls_mutex;

  /**
   * If the execution job should be executed asynchronously. If set to false,
   * the service's execution is bound to the job execution cycle and can
   * potentially block it.
   *
   * @attention Small jobs should be synchronous and bound to the job execution
   * cycle. Big ones (potentially chaining other foreign service calls) which
   * can take longer to resolve should be asynchronous to not block the job
   * execution cycle.
   */
  bool m_async;

public:
  LocalServiceExecutor() = delete;

  /**
   * Constructor
   * @param service_name name of service to execute
   * @param func functor containing the actual service
   * @param async If the execution job should be asynchronous. Small jobs should
   * be synchronous and bound to the job execution cycle. Big ones (potentially
   * chaining other foreign service calls) which can take longer to resolve
   * should be asynchronous to not block the job execution cycle.
   * @param capacity limit of concurrent calls to this service
   * (negative means infinite).
   * @attention if the limit is reached, the call will return a busy response.
   */
  explicit LocalServiceExecutor(std::string service_name,
                                service_functor_t func, bool async = false,
                                size_t capacity = -1);
  ~LocalServiceExecutor() override = default;

  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool async) override;

  capacity_t GetCapacity() const override;
  bool IsCallable() override;
  std::string GetServiceName() override;
  bool IsLocal() override;
  std::string GetId() override;
};

inline bool LocalServiceExecutor::IsCallable() { return true; }
inline capacity_t LocalServiceExecutor::GetCapacity() const {
  return m_capacity;
}
inline std::string LocalServiceExecutor::GetServiceName() {
  return m_service_name;
}
inline bool LocalServiceExecutor::IsLocal() { return true; }
inline std::string LocalServiceExecutor::GetId() { return m_id; }

} // namespace hive::services::impl
