#ifndef LOCALONLYSERVICEREGISTRY_H
#define LOCALONLYSERVICEREGISTRY_H

#include "services/Services.h"
#include "services/registry/IServiceRegistry.h"
#include <future>
#include <map>

using namespace services;

namespace services::impl {

/**
 * A registry for services that can be called directly.
 * @attention This registry does not have any registration, request-response
 * implementations for remote service calls. It is only meant for local
 * services.
 */
class SERVICES_API LocalOnlyServiceRegistry : public IServiceRegistry {
protected:
  std::map<std::string, SharedServiceCaller> m_registered_services;

public:
  void Register(const SharedServiceExecutor &stub) override;

  void Unregister(const std::string &name) override;

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
};
} // namespace services::impl

#endif // LOCALONLYSERVICEREGISTRY_H
