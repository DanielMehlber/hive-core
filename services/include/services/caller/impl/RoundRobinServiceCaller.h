#ifndef ROUNDROBINSERVICECALLER_H
#define ROUNDROBINSERVICECALLER_H

#include "services/caller/IServiceCaller.h"
#include <vector>

namespace services::impl {

/**
 * Calls service stubs in sequential order every time a call is requested.
 */
class RoundRobinServiceCaller : public IServiceCaller {
private:
  /** List of service stubs */
  std::vector<SharedServiceStub> m_service_stubs;

  /** Last selected index (necessary for round robin) */
  size_t m_last_index{0};

  std::optional<SharedServiceStub> SelectNextUsableCaller(bool only_local);

public:
  std::future<SharedServiceResponse>
  Call(SharedServiceRequest request, jobsystem::SharedJobManager job_manager,
       bool only_local) noexcept override;

  bool IsCallable() const noexcept override;

  bool ContainsLocallyCallable() const noexcept override;

  void AddServiceStub(SharedServiceStub stub) override;
};

} // namespace services::impl

#endif // ROUNDROBINSERVICECALLER_H
