#ifndef SERVICECALLER_H
#define SERVICECALLER_H

#include "common/exceptions/ExceptionsBase.h"
#include "jobsystem/manager/JobManager.h"
#include "services/stub/IServiceStub.h"
#include <future>

namespace services {

DECLARE_EXCEPTION(NoCallableServiceFound);

/**
 * Selects a service stub for a specific service name and calls it. A caller is
 * necessary to model the 1:n relationship between a service call and its
 * service stubs: A service can be provided by multiple service stubs (e.g. by
 * different hosts in the network using web-sockets). Therefore the caller must
 * select a service stub given a strategy (that can be used for load-balancing).
 */
class IServiceCaller : public std::enable_shared_from_this<IServiceCaller> {
public:
  /**
   * Selects a service stub from a possible set of stubs.
   * @note Executed as job
   * @param request represents input parameters for the service call
   * @param job_manager manages and executes spawned job later
   * @param only_local if only local services should be considered for execution
   * @return future response from service
   */
  virtual std::future<SharedServiceResponse>
  Call(SharedServiceRequest request, jobsystem::SharedJobManager job_manager,
       bool only_local = false) noexcept = 0;

  /**
   * Checks if there are callable services
   * @return true, if there are some callable services
   */
  virtual bool IsCallable() const noexcept = 0;

  /**
   * Checks if caller contains a service executor which is located in the
   * currently running instance (callable by direct call instead of network)
   * @return true, if there is a callable service which is local
   */
  virtual bool ContainsLocallyCallable() const noexcept = 0;

  /**
   * Adds service stub to collection of service stubs.
   * @param stub stub to add
   */
  virtual void AddServiceStub(SharedServiceStub stub) = 0;

  /**
   * @return current count of executable services
   */
  virtual size_t GetCallableCount() const noexcept = 0;
};

typedef std::shared_ptr<IServiceCaller> SharedServiceCaller;

} // namespace services

#endif // SERVICECALLER_H
