#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "common/memory/ExclusiveOwnership.h"
#include "jobsystem/manager/JobManager.h"
#include "services/executor/IServiceExecutor.h"
#include <future>

namespace services {

DECLARE_EXCEPTION(NoCallableServiceFound);

/**
 * A service can be provided by multiple both local and remote executors at
 * once. A service caller selects the next execution to call (given some
 * strategy) to ensure performance and fairness among service executors.
 */
class IServiceCaller {
public:
  /**
   * Selects a service execution from all listed executions given some strategy
   * and call it. Fairness among executors is important to distribute the
   * workload evenly.
   * @note Executed as job in the job-system.
   * @param request represents input parameters for the service call
   * @param job_manager manages and executes spawned job later
   * @param only_local if only local services should be considered for execution
   * @param async if the call should be executed asynchronously. If executed
   * synchronously, it will block the execution cycle until the request has been
   * resolved.
   * @return future response from service
   */
  virtual std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool only_local = false, bool async = false) noexcept = 0;

  /**
   * Checks if there are currently callable and usable service executions
   * available.
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
   * Adds service executor to this caller. There must be no duplicate executors
   * (e.g. two different remote service executors referring to the same node
   * address). If an executor is already registered, it will be replaced.
   * @param executor must not be a duplicate of an already added executor.
   */
  virtual void AddExecutor(SharedServiceExecutor executor) = 0;

  /**
   * @return current count of executable/callable services
   */
  virtual size_t GetCallableCount() const noexcept = 0;
};

typedef std::shared_ptr<IServiceCaller> SharedServiceCaller;

} // namespace services
