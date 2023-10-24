#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include "logging/LogManager.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"

using namespace services::impl;

void LocalOnlyServiceRegistry::Register(const SharedServiceStub &stub) {

  std::string name = stub->GetServiceName();

  if (!m_registered_services.contains(name)) {
    m_registered_services[name] = std::make_shared<RoundRobinServiceCaller>();
  }

  m_registered_services.at(name)->AddServiceStub(stub);

  LOG_DEBUG("new service '" << name
                            << "' has been registered in local registry");
}

void LocalOnlyServiceRegistry::Unregister(const std::string &name) {
  if (m_registered_services.contains(name)) {
    m_registered_services.erase(name);
    LOG_DEBUG("service '" << name
                          << "' has been unregistered from local registry");
  }
}

std::future<std::optional<SharedServiceCaller>>
LocalOnlyServiceRegistry::Find(const std::string &name,
                               bool only_local) noexcept {
  std::promise<std::optional<SharedServiceCaller>> promise;
  std::future<std::optional<SharedServiceCaller>> future = promise.get_future();

  if (m_registered_services.contains(name)) {
    SharedServiceCaller caller = m_registered_services.at(name);
    if (caller->IsCallable()) {
      if (!(only_local) && caller->ContainsLocallyCallable()) {
        promise.set_value(caller);
      } else {
        LOG_WARN("service '" << name
                             << "' is registered in local registry, but not "
                                "locally callable");
        promise.set_value({});
      }
    } else {
      LOG_WARN(
          "service '"
          << name
          << "' was registered in local registry, but is not usable anymore");
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}