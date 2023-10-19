#ifndef LOCALSERVICEREGISTRY_H
#define LOCALSERVICEREGISTRY_H

#include "services/registry/IServiceRegistry.h"
#include <future>
#include <map>

using namespace services;

namespace services::impl {

/**
 * Contains services that have been registered on this host
 */
class LocalServiceRegistry : public IServiceRegistry {
protected:
  std::map<std::string, SharedServiceStub> m_registered_services;

public:
  void Register(const std::string &name,
                const SharedServiceStub &stub) override;
  void Unregister(const std::string &name) override;
  std::future<std::optional<SharedServiceStub>>
  Find(const std::string &name) noexcept override;
};
} // namespace services::impl

#endif // LOCALSERVICEREGISTRY_H
