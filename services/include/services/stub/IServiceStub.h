#ifndef ISERVICESTUB_H
#define ISERVICESTUB_H

#include "jobsystem/manager/JobManager.h"
#include "services/ServiceRequest.h"
#include "services/ServiceResponse.h"
#include <future>

namespace services {

/**
 * @brief Generic interface for services, which act as service implementation.
 * It can be called directly (when located on the same agent) or using network
 * protocols (when located on a remote host).
 */
class IServiceStub : public std::enable_shared_from_this<IServiceStub> {
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
  virtual bool IsCallable() = 0;

  /**
   * @return service name of this stub
   */
  virtual std::string GetServiceName() = 0;

  /**
   * @return true, if service is located on this instance (callable using direct
   * call)
   */
  virtual bool IsLocal() = 0;
};

typedef std::shared_ptr<IServiceStub> SharedServiceStub;
} // namespace services

#endif // ISERVICESTUB_H
