#ifndef LOCALSERVICESTUB_H
#define LOCALSERVICESTUB_H

#include "services/stub/IServiceStub.h"
#include <functional>

using namespace services;

namespace services::impl {

/**
 * Service that will be executed using jobs in this running
 * process.
 */
class LocalServiceStub : public IServiceStub {
protected:
  /**
   * Function that will be called to execute this service
   */
  std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
      m_func;

  std::string m_service_name;

public:
  LocalServiceStub() = delete;
  explicit LocalServiceStub(
      std::string service_name,
      std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
          func);
  virtual ~LocalServiceStub() = default;

  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) override;

  bool IsCallable() override { return true; };

  std::string GetServiceName() override;

  bool IsLocal() override { return true; };
};

} // namespace services::impl

#endif // LOCALSERVICESTUB_H
