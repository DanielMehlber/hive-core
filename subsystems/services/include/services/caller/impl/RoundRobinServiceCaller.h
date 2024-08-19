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

  /** service name of contained executors */
  std::string m_service_name;

  /**
   * Builds a job for calling the service.
   * @param request request that will be passed to the selected executor
   * @param promise promise that resolves the response future passed to the
   * caller
   * @param job_manager job manager that will execute the call job
   * @param retry_on_busy behavior to apply if an executor refuses processing
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
  jobsystem::SharedJob BuildServiceCallJob(
      SharedServiceRequest request,
      std::shared_ptr<std::promise<SharedServiceResponse>> promise,
      common::memory::Borrower<jobsystem::JobManager> job_manager,
      CallRetryPolicy retry_on_busy, bool only_local = false,
      bool async = false,
      std::optional<SharedServiceExecutor> maybe_executor = {},
      int attempt_number = 1);

public:
  RoundRobinServiceCaller() = delete;
  explicit RoundRobinServiceCaller(std::string service_name);

  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool only_local, bool async,
                 CallRetryPolicy retry_on_busy) override;

  bool IsCallable() const override;

  bool ContainsLocallyCallable() const override;

  void AddExecutor(const SharedServiceExecutor &executor) override;
  void RemoveExecutor(const SharedServiceExecutor &executor) override;
  void RemoveExecutor(const std::string &executor_id) override;

  size_t GetCallableCount() const override;
  capacity_t GetCapacity(bool local_only) override;

  std::optional<SharedServiceExecutor>
  SelectNextCallableExecutor(bool only_local) override;

  std::vector<SharedServiceExecutor>
  GetCallableServiceExecutors(bool local_only) override;
};

} // namespace hive::services::impl
