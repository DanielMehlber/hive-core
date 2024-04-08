#ifndef ISERVICEEXECUTOR_H
#define ISERVICEEXECUTOR_H

#include "common/memory/ExclusiveOwnership.h"
#include "jobsystem/manager/JobManager.h"
#include "services/ServiceRequest.h"
#include "services/ServiceResponse.h"
#include <future>

namespace services {

/**
 * Generic interface for service executors which represent a single
 * service implementation
 * @note services can be executed locally or remotely on another machine.
 */
class IServiceExecutor {
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
  IssueCallAsJob(SharedServiceRequest request,
       common::memory::Borrower<jobsystem::JobManager> job_manager) = 0;

  /**
   * Checks if the service can be currently called
   * @return true, if service can be called
   */
  virtual bool IsCallable() = 0;

  /**
   * @return name of this executor's service
   */
  virtual std::string GetServiceName() = 0;

  /**
   * @return true, if service is located on this node (callable using direct
   * call).
   * @note services can be located on the same node (direct call) or on another
   * (remote call).
   */
  virtual bool IsLocal() = 0;

  /**
   * Return unique ID of the underlying logical service. This allows services of
   * the same name to be distinguished from one another (e.g. by the caller).
   * @note Multiple executors referring to the same logical service must carry
   * the same ID. For instance, remote service executors calling the same node's
   * service must have the same id.
   * @return id of the logical service.
   */
  virtual std::string GetId() = 0;
};

typedef std::shared_ptr<IServiceExecutor> SharedServiceExecutor;
} // namespace services

#endif // ISERVICEEXECUTOR_H
