#ifndef LOCALONLYSERVICEREGISTRY_H
#define LOCALONLYSERVICEREGISTRY_H

#include "services/registry/IServiceRegistry.h"
#include <future>
#include <map>

using namespace services;

namespace services::impl {

/**
 * Contains services that have been registered on this host
 */
class LocalOnlyServiceRegistry : public IServiceRegistry {
protected:
  std::map<std::string, SharedServiceCaller> m_registered_services;

public:
  void Register(const SharedServiceStub &stub) override;

  void Unregister(const std::string &name) override;

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
};
} // namespace services::impl

#endif // LOCALONLYSERVICEREGISTRY_H
