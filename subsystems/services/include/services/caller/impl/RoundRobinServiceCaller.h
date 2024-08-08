#pragma once

#include "services/caller/IServiceCaller.h"
#include <vector>

namespace hive::services::impl {

/**
 * Calls service stubs in sequential order every time a call is requested.
 */
class RoundRobinServiceCaller
    : public IServiceCaller,
      public std::enable_shared_from_this<RoundRobinServiceCaller> {
private:
  /** List of service stubs */
  mutable jobsystem::mutex m_service_executors_mutex;
  std::vector<SharedServiceExecutor> m_service_executors;

  /** Last selected index (necessary for round robin) */
  size_t m_last_index{0};

  /**
   * Builds a job for calling the service.
   * @param request request that will be passed to the selected executor
   * @param promise promise that resolves the response future passed to the
   * caller
   * @param job_manager job manager that will execute the call job
   * @param busy_behavior behavior to apply if an executor refuses processing
   * the request because it is too busy
   * @param only_local if true, only local jobs (running on the same instance)
   * will be considered and called. Remote services will be ignored.
   * @param async if true, the call will be executed asynchronously without
   * blocking the job execution cycle.
   * @param maybe_executor if set, this executor will be used for the call and a
   * new one will not be selected. Used when the call is retried on the same
   * executor.
   * @param attempt_number current count of attempts to resolve the request.
   * Used to limit retry attempts.
   * @return
   */
  SharedJob BuildServiceCallJob(
      SharedServiceRequest request,
      std::shared_ptr<std::promise<SharedServiceResponse>> promise,
      common::memory::Borrower<jobsystem::JobManager> job_manager,
      ExecutorBusyBehavior busy_behavior, bool only_local = false,
      bool async = false,
      std::optional<SharedServiceExecutor> maybe_executor = {},
      int attempt_number = 0);

public:
  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool only_local, bool async,
                 ExecutorBusyBehavior busy_behavior) override;

  bool IsCallable() const override;

  bool ContainsLocallyCallable() const override;

  void AddExecutor(SharedServiceExecutor executor) override;

  size_t GetCallableCount() const override;

  std::optional<SharedServiceExecutor>
  SelectNextCallableExecutor(bool only_local) override;

  std::vector<SharedServiceExecutor>
  GetCallableServiceExecutors(bool local_only) override;
};

} // namespace hive::services::impl
