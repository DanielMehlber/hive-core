#ifndef ISERVICESTUB_H
#define ISERVICESTUB_H

#include "../ServiceRequest.h"
#include "../ServiceResponse.h"
#include "jobsystem/manager/JobManager.h"
#include <future>

namespace services {

/**
 * @brief Generic interface for service stubs, which act like a handle to the
 * actual service implementation. Service Implementation can be called directly
 * (when located on the same agent) or using network protocols (when located on
 * a remote host).
 */
class IServiceStub {
public:
  /**
   * Calls the service using parameters and returns a response eventually
   * @param request request containing parameters and metadata
   * @param job_manager job manager that the
   * @note Service execution is coupled to the job system for parallel service
   * execution
   * @return future response (if the service has completed successfully)
   */
  virtual std::future<SharedServiceResponse>
  Call(SharedServiceRequest request,
       jobsystem::SharedJobManager job_manager) = 0;

  /**
   * Checks if the service can be used currently
   * @return true, if service can be called
   */
  virtual bool IsUsable() = 0;
};

typedef std::shared_ptr<IServiceStub> SharedServiceStub;
} // namespace services

#endif // ISERVICESTUB_H
