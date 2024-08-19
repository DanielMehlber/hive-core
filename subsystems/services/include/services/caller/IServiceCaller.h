#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "common/memory/ExclusiveOwnership.h"
#include "jobsystem/manager/JobManager.h"
#include "services/caller/CallRetryPolicy.h"
#include "services/executor/IServiceExecutor.h"
#include <future>
#include <vector>

namespace hive::services {

DECLARE_EXCEPTION(NoCallableServiceFound);

/**
 * A service can be provided by multiple both local and remote executors at
 * once. A service caller selects the next execution to call (given some
 * strategy) to ensure performance and fairness among service executors.
 */
class IServiceCaller {
public:
  /**
   * Selects a service execution from all listed executions given some
   * strategy and call it. Fairness among executors is important to
   * distribute the workload evenly.
   * @note Executed as job in the job-system.
   * @param request represents input parameters for the service call
   * @param job_manager manages and executes spawned job later
   * @param only_local if only local services should be considered for
   * execution
   * @param async if the call should be executed asynchronously. If
   * executed synchronously, it will block the execution cycle until the
   * request has been resolved.
   * @param retry_on_busy behavior to implement if the called executor
   * is busy.
   * @return future response from service
   */
  virtual std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool only_local = false, bool async = false,
                 CallRetryPolicy retry_on_busy = RETRY_POLICY_NONE) = 0;

  /**
   * Checks if there are currently callable and usable service executions
   * available.
   * @return true, if there are some callable services
   */
  virtual bool IsCallable() const = 0;

  /**
   * Checks if caller contains a service executor which is located in the
   * currently running instance (callable by direct call instead of network)
   * @return true, if there is a callable service which is local
   */
  virtual bool ContainsLocallyCallable() const = 0;

  /**
   * Adds service executor to this caller. There must be no duplicate
   * executors (e.g. two different remote service executors referring to the
   * same node address). If an executor is already registered, it will be
   * replaced.
   * @param executor must not be a duplicate of an already added executor.
   */
  virtual void AddExecutor(const SharedServiceExecutor &executor) = 0;

  /**
   * Removes a service executor with a given ID from this caller.
   * @param executor_id id of the service executor.
   */
  virtual void RemoveExecutor(const std::string &executor_id) = 0;

  /**
   * Removes a service executor from this caller.
   * @param executor executor to remove from the caller.
   */
  virtual void RemoveExecutor(const SharedServiceExecutor &executor) = 0;

  /**
   * @return current count of executable/callable services
   */
  virtual size_t GetCallableCount() const = 0;

  /**
   * Returns the total summed-up capacity (in parallel processable service
   * requests) of all executors callable by this instance.
   * @param local_only only consider local services for capacity calculation
   * @return total capacity of all known and callable executors
   */
  virtual capacity_t GetCapacity(bool local_only = false) = 0;

  /**
   * @param local_only if only local services should be considered for
   * execution
   * @return next executor in line to be called (if one exists). The next
   * executor is selected by a strategy of the underlying implementation.
   */
  virtual std::optional<SharedServiceExecutor>
  SelectNextCallableExecutor(bool local_only = false) = 0;

  virtual std::vector<SharedServiceExecutor>
  GetCallableServiceExecutors(bool local_only = false) = 0;

  virtual ~IServiceCaller() = default;
};

typedef std::shared_ptr<IServiceCaller> SharedServiceCaller;

} // namespace hive::services
