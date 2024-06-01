#ifndef ROUNDROBINSERVICECALLER_H
#define ROUNDROBINSERVICECALLER_H

#include "services/caller/IServiceCaller.h"
#include <vector>

namespace services::impl {

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

  std::optional<SharedServiceExecutor> SelectNextUsableCaller(bool only_local);

public:
  std::future<SharedServiceResponse>
  IssueCallAsJob(SharedServiceRequest request,
                 common::memory::Borrower<jobsystem::JobManager> job_manager,
                 bool only_local, bool async) noexcept override;

  bool IsCallable() const noexcept override;

  bool ContainsLocallyCallable() const noexcept override;

  void AddExecutor(SharedServiceExecutor executor) override;

  size_t GetCallableCount() const noexcept override;
};

} // namespace services::impl

#endif // ROUNDROBINSERVICECALLER_H
