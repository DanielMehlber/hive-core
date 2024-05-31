#ifndef LOCALSERVICEEXECUTOR_H
#define LOCALSERVICEEXECUTOR_H

#include "services/executor/IServiceExecutor.h"
#include <functional>

using namespace services;

namespace services::impl {

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
  std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
      m_func;

  /** service name */
  std::string m_service_name;

  /** unique id of this local service executor */
  std::string m_id;

public:
  LocalServiceExecutor() = delete;
  explicit LocalServiceExecutor(
      std::string service_name,
      std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
          func);
  virtual ~LocalServiceExecutor() = default;

  std::future<SharedServiceResponse> IssueCallAsJob(
      SharedServiceRequest request,
      common::memory::Borrower<jobsystem::JobManager> job_manager) override;

  bool IsCallable() override { return true; };

  std::string GetServiceName() override;

  bool IsLocal() override { return true; };

  std::string GetId() override { return m_id; }
};

} // namespace services::impl

#endif // LOCALSERVICEEXECUTOR_H
