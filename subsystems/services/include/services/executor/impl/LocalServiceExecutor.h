#ifndef LOCALSERVICEEXECUTOR_H
#define LOCALSERVICEEXECUTOR_H

#include "services/Services.h"
#include "services/executor/IServiceExecutor.h"
#include <functional>

using namespace services;

namespace services::impl {

/**
 * Executes services using a direct function call.
 * @attention service must be located on the same node. This executor does not
 * support remote service calls.
 */
class SERVICES_API LocalServiceExecutor
    : public IServiceExecutor,
      public std::enable_shared_from_this<LocalServiceExecutor> {
protected:
  /**
   * Function that will be called to execute this service
   */
  std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
      m_func;

  std::string m_service_name;

public:
  LocalServiceExecutor() = delete;
  explicit LocalServiceExecutor(
      std::string service_name,
      std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
          func);
  virtual ~LocalServiceExecutor() = default;

  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) override;

  bool IsCallable() override { return true; };

  std::string GetServiceName() override;

  bool IsLocal() override { return true; };
};

} // namespace services::impl

#endif // LOCALSERVICEEXECUTOR_H
